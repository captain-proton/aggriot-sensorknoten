#include "communication.h"
#include "ringBuffer.h"
#include "uart.h"
#include <avr/interrupt.h>

#define USE_CRC16
#define USE_SIGNATURES

#ifndef SYNCBYTE
#define SYNCBYTE	249
#endif

#define BITS_VERSION_0		0b00
#define BIT_A_TO_S				1
#define BIT_S_TO_A				0
#define BIT_IS_ACK				1
#define FLAG_IS_ACK				0b100000
#define FLAG_A_TO_S				0b010000

#define RF_UART_IN_TIMEOUT_MS		25 // Darf nicht viel h�her sein. Sonst werden Module falsch synchronisiert weil die Nachrichten "aufgehalten" werden


unsigned short rf_uart_put_and_crc(unsigned short crc, uint8_t sendchar);

static uint8_t rxlen = 0;

static uint32_t last_messageorigin = 0;
static uint8_t last_message_cmd_sequence = 0;
static uint16_t last_crc_sum = 0;
static uint8_t mycurrentseq = 0;
static uint32_t timeOfArrival;


static uint8_t executingComCheck = 0;
RB_BUFFER_t * inBuffer;
static UART_INFO_t * uarts[2];
static uint8_t in_timeout_ms = 0;



void communication_init() {
	static uint8_t inBuffer_space[256];
	// Puffer initialisieren:
	inBuffer = rb_createBuffer(&inBuffer_space[0], sizeof(inBuffer_space));
}

uint32_t communication_getCommandReferenceTime() {
	return timeOfArrival;
}

uint8_t communication_dataIn(void * infoPtr, uint8_t event, uint8_t data) {
	if (event == UART_EVENT_RECEIVED) {
		setDebugStep(105);
		if (inBuffer) {
			if (rb_put(inBuffer, data)) {
				if (data == SYNCBYTE) {
					toab_addEvent();
				} else {
					toab_addCount();
				}
				setDebugStep(106);
			} else {
				setDebugStep(107);
			}
		}
		in_timeout_ms = RF_UART_IN_TIMEOUT_MS;
	}
	return 1;
}

void communication_connectUART(UART_INFO_t * uart) {
	// Notify-Funktion registrieren:
	uart_addModifyNotifyFunction(uart, 0, communication_dataIn, 0xffff);
	if (!uarts[0]) {
		uarts[0] = uart;
		return;
	}
	if (!uarts[1]) {
		uarts[1] = uart;
		return;
	}
}





void communication_check_uart(void) { // Funktion wird regelm��ig aufgerufen
	uint16_t rxc;
	uint8_t cmdlen;
	uint8_t forme;
	unsigned short mycrc;
	uint8_t i;
	uint8_t p;
	uint8_t dbl_msg = 0;
	
	MESSAGE_PARAM_t m;
	
	uint8_t cmd_sequence;
	
	// Puffer �berpr�fen:
	while ((rxc = rb_getCount_sync(inBuffer))) {
		if (SYNCBYTE == rb_peek_sync(inBuffer, 0)) {
			// Danach kommt die L�nge der eigentlichen Information. Falls die Angabe hier < 4 ist,
			// ist der Payload 4 Byte lang aber nur ein Teil davon ist relevant f�r den Nutzer.
			if (rxc > 1) {
				
				// L�ngeninformation ist zweites Byte:
				cmdlen = rb_peek_sync(inBuffer, 1);
				uint16_t packetLength = (uint16_t)cmdlen + max(cmdlen, 4) + sizeof(MessageHeader) + sizeof(MessageFooter); // +1 Sync-Byte
				
				if (rxc >= packetLength) {
					//if ((uint8_t)(cmdlen + 4) <= rxc) { // Die 4 Bytes SYNCBYTE, L�NGE, CRCHIGH und CRCLOW noch einrechnen
						setDebugStep(120);
						
						// Zuerst die komplette CRC berechnen. Wenn wir einem unechten SYNC aufgesessen sind,
						// sollte das verhindern, das zu viele der echten Zeichen weggeworfen werden.
						// Auf diese Art kann der dahinter kommende, "echte" Befehl noch ausgewertet werden:
						mycrc = crc_calcCRC16r(0xffff, cmdlen);
						
						for (i=2;i<packetLength;i++)
							mycrc = crc_calcCRC16r(mycrc, rb_peek_sync(inBuffer, i));

						if (((mycrc >> 8) == rb_peek_sync(inBuffer, packetLength - 1)) && (((uint8_t)mycrc) == rb_peek_sync(inBuffer, packetLength + 2))) {
							// Korrekt empfangen. Paketdaten in die Headerstrukturen kopieren:
							MessageHeader mHdr;
							MessageFooter mFtr;
							uint8_t messageData[max(cmdlen, 4)];
							uint8_t len = sizeof(MessageHeader);
							uint16_t p = 0;
							uint8_t * ptr = &mHdr;
							
							// Daten im Header kopieren:
							while (len--)
								*ptr++ = rb_peek_sync(inBuffer, p++);
							
							// Daten selbst kopieren:
							len = sizeof(messageData);
							ptr = &messageData[0];
							while (len--)
								*ptr++ = rb_peek_sync(inBuffer, p++);
							
							// Daten im Footer kopieren:
							ptr = &mFtr;
							len = sizeof(MessageFooter);
							while (len--)
								*ptr++ = rb_peek_sync(inBuffer, p++);
							
							// Daten aus dem Eingangspuffer l�schen:
							rb_delete_sync(inBuffer, packetLength);
							
							// Pr�fen ob wir der Empf�nger sind:
							if ((mHdr.flags >> 6) == BITS_VERSION_0) {
								if (mHdr.flags & FLAG_A_TO_S) {
									// Eingehende Nachricht zu einem Sensor. Sind wir gemeint?
									if (mHdr.sensorAddress == MY_SENSOR_ADDRESS) {
										
										// Versuchen die Nachricht zu entschl�sseln:
										communication_cryptPayload(&messageData[0], sizeof(messageData), mHdr.sequenceNumber, 1); // 1 = Incoming message
										
										uint32_t calculatedCRC = 0xffffffff;
										// Testen, ob die Pr�fsumme jetzt zu den entschl�sselten Daten passt:
										for (uint8_t i=0;i<sizeof(messageData);i++) {
											calculatedCRC = crc_calcCRC32(calculatedCRC, messageData[i]);
										}
										
										// Ist korrekt ver/entschl�sselt (und �bertragen) worden?
										if (calculatedCRC == mFtr.payloadCRC) {
											
											
											
											
											
										}
									}
								}
							}
						} else {
							// Die CRC war falsch. Eventuell war das garkein Befehl auf den wir reagiert haben!
							// Also einfach das "falsche" SYNC wegwerfen und nach einem neuen SYNC suchen:
							rb_delete_sync(inBuffer, 1);
						}
						// N�chsten Befehl abarbeiten..
						continue;
					}
				} else {
					// Das gedachte Sync-Zeichen war kein Sync!
					rb_delete_sync(inBuffer, 1);
					continue;
				}
			}
			
			// Der Befehl ist noch nicht vollst�ndig. Wenn aber die einzelnen Zeichen eines Befehls mehr als (X)ms auseinander liegen,
			// wird der Befehl abgebrochen da wir evtl ein nicht-sync-Zeichen als SYNC interpretiert haben weil das Modul
			// den Anfang des echten Befehls nicht mitbekommen hat. In dem Fall �bergehen wir das SYNC-Zeichen und laufen direkt zum
			// n�chsten SYNC-Byte.
			if (!in_timeout_ms) {
				// SYNC wegwerfen:
				rb_delete_sync(inBuffer, 1);
				continue; // Und gleich beim n�chsten Byte gucken ob das ein SYNC-Byte sein k�nnte..
			} else {
				// Wir laufen einmal pro Millisekunde. Perfekt also um den Z�hler runter zu z�hlen wenn das n�tig ist.
				in_timeout_ms--; // Tick, tick..
				break;
			}
		} else {
			// Zeichen wird verworfen:
			rb_delete_sync(inBuffer, 1);
		}
	}
	
	executingComCheck = 0;
}

void communication_sendCommand(uint8_t* data, uint8_t data_len, MESSAGE_PARAM_t m) {
/*
struct message_param {
 	uint32_t	from_addr;
 	uint32_t	to_addr;
	uint8_t		addr_mode;
	uint16_t	system_code;
	uint8_t		status_info;
	uint8_t		from_dtype;
	uint8_t		to_dtype;
	uint8_t		is_signed;
	uint8_t		is_broadcast;
};
*/	
	
	uint8_t preamble[SIGNATURE_SEQUENCE_LEN + 15];
#ifdef USE_CRC16
	unsigned short mycrc = 0;
#else
	uint8_t mycrc = 0;
#endif
	uint8_t k = 0;
	uint8_t i = 0;
	uint8_t j = 0;
	uint8_t cmdlen = 0;
	
	// Nachricht ist so aufgebaut: 
	 // <Sync 1><Len 1><AddrMode 1><Cmd_Sequence 4 Bit + LastCmdStatus 4 Bit><StatusInfo 1><Befehlsstatus 1>[<System Code 2>]<To Addr 2/3><From Addr 2/3><Sequence><Hash><Message><CRC>
	
	if (data_len > MAXMESSAGELENGTH) {
		data_len = MAXMESSAGELENGTH;
	}
	
	j = 0;
	// preamble[j++] = SYNCBYTE;					// SYNC	Werden hinterher separat gesendet..
	// preamble[j++] = 0;							// LEN  ..
	preamble[j++] = 0;							// HEADERS
	preamble[j++] = m.command_status & 0xf;		// SEQUENCE + CMD-STATUS
	preamble[j++] = m.status_info;				// STATUS-INFO
	
	
	if (m.to_addr >> 16) { // Wenn die Zieladresse bereits breiter als 16 bit ist dann kann als Adressierungsformat nur die ID in Frage kommen..
		// Sicherstellen das genau diese Adressierung benutzt wird..
		m.addr_mode &= ~(1 << CMD_HEADERPRESENT_TO_ADDRESS_NUMBER);
	}
	// Das gleiche gilt nat�rlich auch f�r die Absenderadresse:
	if (m.from_addr >> 16) {
		m.addr_mode &= ~(1 << CMD_HEADERPRESENT_FROM_ADDRESS_NUMBER);
	}
	
	
#ifdef USE_SIGNATURES
	m.is_signed |= m.addr_mode & (1 << CMD_HEADERPRESENT_SIGNATURE);
	if (m.is_signed) {
		m.addr_mode |= (1 << CMD_HEADERPRESENT_SIGNATURE);
		// ACHTUNG: Ein signierter Befehl darf nur im Kontext der ModulID stehen, oder muss den Systemcode beinhalten:
		if (m.addr_mode & (1 << CMD_HEADERPRESENT_TO_ADDRESS_NUMBER)) {
			m.addr_mode |= (1 << CMD_HEADERPRESENT_SYSTEMCODE);
		}
	}
#else
	m.is_signed = 0;
	m.addr_mode &= ~(1 << CMD_HEADERPRESENT_SIGNATURE);
#endif
	
#ifdef INCLUDE_MESSAGE_DEBUG_LOG
	mdbug_addOutgoingMessage(&m, data, data_len);
#endif
	
	if (m.addr_mode & (1 << CMD_HEADERPRESENT_SYSTEMCODE)) {
		preamble[j++] = (uint8_t)(m.system_code >> 8);
		preamble[j++] = (uint8_t)m.system_code;
	}
	// So, Zieladresse einbauen:
	if (m.addr_mode & (1 << CMD_HEADERPRESENT_TO_ADDRESS_NUMBER)) {
		preamble[j++] = (uint8_t)((m.to_dtype << 4) | (m.to_addr >> 8));
		preamble[j++] = (uint8_t)m.to_addr;
	} else {
		preamble[j++] = (uint8_t)((m.to_dtype << 4) | (m.to_addr >> 16));
		preamble[j++] = (uint8_t)(m.to_addr >> 8);
		preamble[j++] = (uint8_t)(m.to_addr);
	}
	// Hier eine Ausnahme: Die Werte f�r die Quelladresse werden automatisch gef�llt wenn nicht schon ausgef�llt.
	if (m.from_addr || m.from_dtype) {
		if (m.addr_mode & (1 << CMD_HEADERPRESENT_FROM_ADDRESS_NUMBER)) {
			preamble[j++] = (m.from_addr >> 8) | (m.from_dtype << 4);
			preamble[j++] = (uint8_t)(m.from_addr);
		} else {
			preamble[j++] = (m.from_addr >> 16) | (m.from_dtype << 4);
			preamble[j++] = (m.from_addr >> 8);
			preamble[j++] = (uint8_t)(m.from_addr);
		}
	} else {
		if (m.addr_mode & (1 << CMD_HEADERPRESENT_FROM_ADDRESS_NUMBER)) {
			preamble[j++] = (uint8_t)(moduleSettings.moduleNr >> 8);
			preamble[j++] = (uint8_t)(moduleSettings.moduleNr >> 0);
		} else {
			preamble[j++] = (uint8_t)(moduleSettings.moduleID >> 16);
			preamble[j++] = (uint8_t)(moduleSettings.moduleID >> 8);
			preamble[j++] = (uint8_t)(moduleSettings.moduleID >> 0);
		}
	}
		
	// Wenn wir einen anderen Adressraum benutzen sollen (nicht 0), dann f�gen wir hier den entsprechenden Wert ein und setzen das DEVPR-Bit
	if (m.protocolID) {
		preamble[j++] = m.protocolID;
		m.addr_mode |= (1 << CMD_HEADERPRESENT_DEVPR);
	}
	
	cmdlen = j + data_len;
	
	// Wenn wir den Systemstatus mitsenden sollen: Aufbereiten und hinzuf�gen:
	
	
	
	
	
	
	preamble[0] = m.addr_mode;
	preamble[1] |= ((mycurrentseq & 0xF) << 4);
	mycurrentseq++;
	
#ifdef USE_SIGNATURES
	// Dann kommt wenn eine Signtur gew�nscht ist die aktuelle Signatur + 1 zum Einsatz..
	if (m.is_signed) {
		// SHA1 initialisieren
		CryptInit();
		CryptGenerateKey(&moduleSettings.signature_passphrase[0], SIGNATURE_PASSPHRASE_LEN);
		
		// Die Signatur wird +1 gerechnet:
		/*
			
			Die Sequenz WIRD NICHT VER�NDERT! Das f�hrt sonst nur zu Problemen wenn der Commander eine
			Antwort nicht bekommen hat - dann ist das Modul n�mlich in der Sequenznummer weiter als der
			Commander..!
			
			Folgender Fall z.B.: Commander sendet SET_SEQUENCE, Modul f�hrt dies aus und die Antwort wird
			jedoch nicht korrekt �bertragen. Der Commander sendet daraufhin SET_SEQUENCE mit einer ung�ltigen
			Signatur (Modul hat ja weiter gez�hlt!). Das Modul antwortet daraufhin mit WRONG_SIGNATURE, da der
			Hash jedoch g�ltig ist, wird die Antwort g�ltig signiert. Dabei wird wieder +1 gerechnet, so dass
			der n�chste Versuch des Commanders, SET_SEQUENCE durchzuf�hren wieder scheitert. Das GILT F�R ALLE
			FOLGENDEN BEFEHLE DIE AUF DIE SIGNATUR ANGEWIESEN SIND!
			
			Die R�cknachricht muss ja nur signiert sein, wenn der Commander die Nachricht best�tigt haben MUSS.
			Das wird jedoch auch erreicht, wenn die ankommende ID einfach hoch gez�hlt wird.
			
			
		i = SIGNATURE_SEQUENCE_LEN;
		while (i--) {
			if (++signature_sequence[i]) { // Wenn nicht 0 geworden (=> �berlauf) dann wars das bereits
				break;
			}
		}
		*/
		
		i = SIGNATURE_SEQUENCE_LEN;
		k = 0;
		while (i--) {
			preamble[j++] = (m.masterSequenceMode ? signature_sequence_master[k] : signature_sequence[k]);
			k++;
		}
		// Die bisherigen Daten SHA1-Hashen, SYNC und LEN sind au�en vor, da diese sowieso direkt aus der Variablen gesendet werden
		CryptFeedData(&preamble[0], j, 0);
		
		// Die geheime Passphrase in den SHA1-Generator "f�ttern":
		//for (i=0;i<SIGNATURE_PASSPHRASE_LEN;i++) {
		//	CryptFeedOneByte(commanderSettings.signature_passphrase[i]);
		//}
		
		// Danach kommen die eigentlichen Daten. Der Parameter am Ende gibt an dass der Hash auch direkt berechnet werden soll..
		CryptFeedData(data, data_len, 1);
		// Nur wenn die Nachricht signiert verschickt wird rechnen wir den Hash in die Nachrichtenl�nge mit ein:
		cmdlen = j + data_len + HASHLENGTH;
	}
#endif
	
	// Da es beim Funkprotokoll Probleme mit einer zu kurzen Einschwingzeit des RF-Moduls gab werden hier erstmal 2 Leerbytes verschickt. Das dauert ca. 0,5ms
	i = 2;
	while (i--) {
		uart_mgmt_putc_both(0xBB);
	}
	
	// Daten raus senden. Wir fangen mit SYNC an:
	uart_mgmt_putc_both(SYNCBYTE);
	
	// Dann die L�nge des Pakets, die wird schon in die CRC eingerechnet:
#ifdef USE_CRC16
	mycrc = rf_uart_put_and_crc(0xffff, cmdlen);
#else
	mycrc = rf_uart_put_and_crc(0xff, cmdlen);
#endif
	
	// Danach kommt die Preambel
	for (i=0;i<j;i++) {
		mycrc = rf_uart_put_and_crc(mycrc, preamble[i]);
	}
#ifdef USE_SIGNATURES
	// Wenn signiert kommt jetzt die Signatur:
	if (m.is_signed) {
		for (i=0;i<HASHLENGTH;i++) {
			mycrc = rf_uart_put_and_crc(mycrc, crypt_result(i));
		}
	}
#endif
	
	// Und zu guter letzt kommen die Daten:
	while (data_len--) {
		mycrc = rf_uart_put_and_crc(mycrc, *data);
		data++;
	}
	
#ifdef USE_CRC16
	uart_mgmt_putc_both(mycrc >> 8);
	uart_mgmt_putc_both((uint8_t)mycrc);
#else
	uart_mgmt_putc_both(mycrc);
#endif
}

#ifdef USE_CRC16
unsigned short rf_uart_put_and_crc(unsigned short crc, uint8_t sendchar) {
	uart_mgmt_putc_both(sendchar);
	//rf_uart_putc(sendchar);
	return crc_calcCRC16r(crc, sendchar);
}
#else
uint8_t rf_uart_put_and_crc(uint8_t crc, uint8_t sendchar) {
	rf_uart_putc(sendchar);
	return crc_calcCRC8r(crc, sendchar);
}
#endif


typedef union {
	struct {
		uint32_t sensorAddress;
		uint32_t sequenceNumber;
		uint8_t blockNumber;
	};
	uint8_t byteblock[16];
} EncryptionBlock;

void communication_cryptPayload(uint8_t * payload, uint8_t payloadLength, uint32_t sequenceNumber, uint8_t incoming) {
	// Key laden falls noch nicht vorhanden:
	aes_loadKey();
	
	uint8_t blockNumber = incoming ? 0 : 128;
	uint8_t bufferPosition = 0;
	
	do {
		// OTP-Block erzeugen:
		EncryptionBlock block;
		// Zero memory:
		len = 16;
		while (len--)
			block.byteblock[len] = 0;
		
		// Ursprungsdaten einf�llen
		block.sensorAddress = SENSOR_ADDRESS;
		block.sequenceNumber = sequenceNumber;
		block.blockNumber = blockNumber;
		
		// Verschl�sseln:
		aes_encryptBlock(&block);
		
		do {
			if (!payloadLength--)
				return;
			*payload ^= block.byteblock[bufferPosition & 15];
		} while ((++bufferPosition & 15));
	} while (++blockNumber);
}
