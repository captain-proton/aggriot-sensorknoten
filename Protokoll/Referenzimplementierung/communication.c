#include <stdio.h>
#include "aes.h"
#include "communication.h"
#include "ringBuffer.h"

#ifndef TESTING
#include <avr/io.h>
#endif

#define max(x, y)							((x) > (y) ? (x) : (y))

#define CRC32LEN	(sizeof(uint32_t))

uint16_t crc_calcCRC16r(uint16_t crc, uint8_t c) {
  unsigned char i;
  for(i=0;i<8;i++) {
    if ((crc ^ c) & 1) {
    	crc = (crc>>1) ^ CRC16MASK;
    } else {
    	crc	>>=	1;
    }
    c >>=	1;
  }
  return crc;
}

uint32_t crc_calcCRC32r(uint32_t crc, uint8_t c) {
  unsigned char i;
  for(i=0;i<8;i++) {
    if ((crc ^ c) & 1) {
    	crc = (crc>>1) ^ CRC32MASK;
    } else {
    	crc	>>=	1;
    }
    c >>= 1;
  }
  return crc;
}


// For testing:
#ifdef TESTING
#define NO_AVR
#include "ringBuffer.c"
#include "aes.c"
#undef NO_AVR
#else
// Kill all printfs
#define printf(x, ...)		
#endif

RB_BUFFER_t * inBuffer;									// Eingangspuffer
static uint8_t executingCOMCheck = 0;		// Wird communication_poll gerade ausgeführt?
static uint32_t lastByteReceived = 0; 			// Timeout für eingehende Bytes
static uint32_t unAckedMessage;					// Welche Sequenznummer hat die unbestätigte Nachricht die noch unterwegs ist?
static uint16_t messageTimeout;					// Timeout für eine unbestätigte Nachricht
static uint32_t currentSequenceNumber;	// Aktuelle Sequenznummer
static uint32_t simSeqNum;							// Simulierte Sequenznummer des Gegenübers
static uint8_t currentRole;
static uint32_t myAddress;							// Meine Adresse
#define ROLE_SENSOR			0
#define ROLE_AGGREGATOR	1


void com_messageReceived(MessageHeader * mHdr, uint8_t * payload, uint8_t payloadLength);
void com_processValidMessage(uint8_t * payload, uint8_t payloadLength);
void com_messageTimeout();
uint8_t com_sendMessage(uint8_t * data, uint8_t len);

#ifndef TESTING
void com_processValidMessage(uint8_t * payload, uint8_t payloadLength) __attribute__((weak));
void com_messageTimeout() __attribute__((weak));
void com_messageAcked();
void com_sendOutgoingData(uint8_t * ptr, uint8_t length) __attribute__((weak));
uint32_t com_getMillis() __attribute__((weak));
void com_println(char * msg) __attribute__((weak));
#endif

void communication_init(uint32_t sensorAddress) {
	static uint8_t inBuffer_space[256];
	inBuffer = rb_createBuffer(&inBuffer_space[0], sizeof(inBuffer_space));
	if (inBuffer) {
		printf("Input buffer created.\n");
	}
	myAddress = sensorAddress;
}

// Empfangenen Datenblock füttern:
void communication_dataIn(uint8_t * dataPtr, uint8_t len) {
	printf("Received %d bytes..\n", len);
	while (len--)
		rb_put(inBuffer, *dataPtr++);
	lastByteReceived = com_getMillis();
	printf("Last byte received: %u.\n", lastByteReceived);
}

// Regelmäßig nachgucken ob Daten da sind bzw Timeouts zählen
void communication_poll(void) { // Funktion wird regelmäßig (möglichst exakt jede Millisekunde) aufgerufen
	uint16_t rxc;
	uint8_t cmdlen;
	uint8_t i;
	uint16_t mycrc;
	uint32_t timeNow = com_getMillis();
	
#ifndef TESTING
	if (executingCOMCheck)
		return;
	executingCOMCheck = 1;
#endif
	
	// Puffer überprüfen:
	while ((rxc = rb_getCount_sync(inBuffer))) {
		if (SYNCBYTE == rb_peek_sync(inBuffer, 0)) {
			
			// Danach kommt die Länge der eigentlichen Information. Falls die Angabe hier < 4 ist,
			// ist der Payload 4 Byte lang aber nur ein Teil davon ist relevant für den Nutzer.
			if (rxc > 1) {
				
				// Längeninformation ist drittes Byte, nach den Flags:
				cmdlen = rb_peek_sync(inBuffer, 2);
				uint16_t packetLength = max(cmdlen, MIN_DATA_SIZE) + sizeof(MessageHeader) + sizeof(MessageFooter);
				
				if (rxc >= packetLength) {
					
					
					// Zuerst die komplette CRC berechnen. Wenn wir einem unechten SYNC aufgesessen sind,
					// sollte das verhindern, das zu viele der echten Zeichen weggeworfen werden.
					// Auf diese Art kann der dahinter kommende, "echte" Befehl noch ausgewertet werden:
					mycrc = CRC16START;

	if (currentRole) {
		printf("### ROLE: AGGREGATOR %d.\n", currentRole);
	} else {
		printf("### ROLE: SENSOR %d.\n", currentRole);
	}
					
					// CRC berechnen. Ab Startbyte, ohne die eigentliche CRC16-Prüfsumme am Ende:
					printf("RECV: ");
					for (i=0;i<packetLength-2;i++) {
						printf("%.2x ", rb_peek_sync(inBuffer, i));
						mycrc = crc_calcCRC16r(mycrc, rb_peek_sync(inBuffer, i));
					}
					printf("\n");
					
					printf("Calculated CRC %x ", mycrc);
					
					if (((mycrc >> 8) == rb_peek_sync(inBuffer, packetLength-1)) && (((uint8_t)mycrc) == rb_peek_sync(inBuffer, packetLength-2))) {
						printf("is valid.\n");
						
						// Korrekt empfangen. Paketdaten kopieren:
						uint8_t recvData[packetLength];
						uint8_t p = 0;
						for (p=0;p<packetLength;p++)
							recvData[p] = rb_peek_sync(inBuffer, p);
						// Header-Struktur daraus machen:
						MessageHeader * mHdr = (MessageHeader*)&recvData[0];
						uint8_t * messageData = &recvData[sizeof(MessageHeader)];
						MessageFooter * mFtr = (MessageFooter*)&recvData[sizeof(MessageHeader) + max(cmdlen, MIN_DATA_SIZE)];
						
						// Daten aus dem Eingangspuffer löschen:
						rb_delete_sync(inBuffer, packetLength);
						
						// Prüfen ob wir der Empfänger sind:
						if ((mHdr->flags >> 6) == BITS_VERSION_0) {
							if (((mHdr->flags & FLAG_A_TO_S) ? ROLE_SENSOR : ROLE_AGGREGATOR) == currentRole) {
								
								printf("-- Incoming message to sensor 0x%x --\n", mHdr->sensorAddress);
								
								
								// Eingehende Nachricht zu einem Sensor/Aggregator. Sind wir gemeint?
								if ((mHdr->sensorAddress == myAddress) || (currentRole == ROLE_AGGREGATOR)) {
									com_println("   That's me!\n");
									
#ifdef USE_MAC									
									// Versuchen die Nachricht zu entschlüsseln:
									aes_cryptPayload(&messageData[0], max(cmdlen, MIN_DATA_SIZE), mHdr->sensorAddress, mHdr->sequenceNumber, mHdr->flags & FLAG_A_TO_S); // 1 = Incoming message
									
									printf("Payload=");
									for (i=0;i<cmdlen;i++)
										printf("%.2x ", messageData[i]);
									printf("\n");
									
									uint8_t MAC_buffer[MAC_SIZE];
									
									// MAC über komplette Nachricht berechnen (bis auf den MAC selber und die CRC16):
									aes_mac_calculate(&MAC_buffer[0], MAC_SIZE, &recvData[0], sizeof(MessageHeader) + max(cmdlen, MIN_DATA_SIZE));
									
									uint8_t isValid = 1;
									for (i=0;i<MAC_SIZE;i++) {
										if (mFtr->MAC[i] != MAC_buffer[i]) {
											isValid = 0;
											break;
										}
									}
									
									if (isValid) {
#else									
									// Versuchen die Nachricht zu entschlüsseln:
									aes_cryptPayload(&messageData[0], max(cmdlen, MIN_DATA_SIZE)+CRC32LEN, mHdr->sensorAddress, mHdr->sequenceNumber, mHdr->flags & FLAG_A_TO_S); // 1 = Incoming message
									
									uint32_t calculatedCRC = CRC32START;
									// Testen, ob die Prüfsumme jetzt zu den entschlüsselten Daten passt:
									printf(":: CRC32 over %d bytes: ", max(cmdlen, MIN_DATA_SIZE));
									for (i=0;i<max(cmdlen, MIN_DATA_SIZE);i++) {
										printf("%.2x ", messageData[i]);
										calculatedCRC = crc_calcCRC32r(calculatedCRC, messageData[i]);
									}
									printf("\n");
									
									// Ist korrekt ver/entschlüsselt (und übertragen) worden?
									if (calculatedCRC == mFtr->payloadCRC) {
#endif
										com_println("   Valid encryption.\n");
										// Nachricht verarbeiten:
										com_messageReceived(mHdr, &messageData[0], cmdlen);
										
									} else { // .. nicht korrekt verschlüsselt: Nachricht ignorieren.
										com_println("   Invalid encryption. Dropping message.\n");
									}
								} else { // .. Nicht für uns. Ignorieren.
									com_println("   Not for me.");
								}
							} else { // .. Nachricht an den Aggregator. Ignorieren.
								com_println("Upstream/downstream wrong!\n");
							}
						} else { // .. Andere Protokollversion. Die kennen wir nicht. Ignorieren.
							com_println("Wrong version.\n");
						}
						
					} else {
						
						// Die CRC war falsch. Eventuell war das garkein Befehl auf den wir reagiert haben!
						// Also einfach das "falsche" SYNC wegwerfen und nach einem neuen SYNC suchen:
						rb_delete_sync(inBuffer, 1);
						
						com_println("-> CRC is invalid.\n");
						
					}
					
					// Wir haben irgendetwas aus dem Eingangspuffer gelöscht - es könnte also noch ein Befehl vorhanden sein: Nächsten Befehl abarbeiten..
					continue;
					
				} else {
					
					//// Das gedachte Sync-Zeichen war kein Sync! Es sind aber eventuell noch weitere Daten im Eingangspuffer..
					//rb_delete_sync(inBuffer, 1);
					//continue;
					
				}
				
			}
			
			// Der Befehl ist noch nicht vollständig. Wenn aber die einzelnen Zeichen eines Befehls mehr als (X)ms auseinander liegen,
			// wird der Befehl abgebrochen da wir evtl ein nicht-sync-Zeichen als SYNC interpretiert haben weil der Sensor
			// den Anfang des echten Befehls nicht mitbekommen hat. In dem Fall übergehen wir das SYNC-Zeichen und laufen direkt zum
			// nächsten SYNC-Byte.
			printf("Last byte received: %u vs time now=%u.\n", lastByteReceived, timeNow);
			printf("Time diff: %u.\n", (timeNow - lastByteReceived));
			
			if ((timeNow - lastByteReceived) > RF_UART_IN_TIMEOUT_MS) {
				printf("Byte timeout. Dropping start byte.\n");
				// SYNC wegwerfen:
				rb_delete_sync(inBuffer, 1);
				continue; // Und gleich beim nächsten Byte gucken ob das ein SYNC-Byte sein könnte..
			} else {
				break;
			}
		} else {
			// Zeichen wird verworfen:
			rb_delete_sync(inBuffer, 1);
		}
	}
	
	// Timeout für erwartete ACKs:
	if (unAckedMessage) {
		printf("Message timeout vs received: %u vs time now=%u.\n", messageTimeout, timeNow);
		printf("Time diff: %u.\n", (timeNow - messageTimeout));
		if ((timeNow - messageTimeout) > MESSAGE_ACK_TIMEOUT) {
			unAckedMessage = 0;
			com_messageTimeout();
		}
	}
	
	executingCOMCheck = 0;
}

// Einzelne Nachricht formatieren und senden:
void communication_sendCommand(uint32_t nodeAddress, uint8_t* data, uint8_t data_len, uint32_t useSequenceNumber, uint8_t isAck) {
	printf("\n\n===============\nRequest to send message data len %d, seqnum %d (ACK=%d)\n", data_len, useSequenceNumber, isAck);
	
	uint8_t msgBuffer[max(data_len, MIN_DATA_SIZE) + sizeof(MessageHeader) + sizeof(MessageFooter)];
	MessageHeader * mHdr = (MessageHeader*)&msgBuffer[0];
	MessageFooter * mFtr = (MessageFooter*)&msgBuffer[sizeof(MessageHeader) + max(data_len, MIN_DATA_SIZE)];
	uint8_t * dataPtr = &msgBuffer[sizeof(MessageHeader)];
	
	// Wir senden mindestens 4 Bytes und füllen unbenutzte Bytes mit \0:
	dataPtr[0] = 0;
	dataPtr[1] = 0;
	dataPtr[2] = 0;
	dataPtr[3] = 0;
	
	mHdr->sync = SYNCBYTE;
	mHdr->flags = (isAck ? FLAG_IS_ACK : 0) | (currentRole == ROLE_AGGREGATOR ? FLAG_A_TO_S : 0)  ;
	mHdr->payloadLength = data_len;
	mHdr->sensorAddress = nodeAddress;
	mHdr->sequenceNumber = useSequenceNumber;
	
	// So viele Daten kopieren wie wir haben, gleichzeitig die Pre-Crypt-Prüfsumme berechnen:
	uint8_t * ptrA = dataPtr;
	uint8_t len = max(data_len, MIN_DATA_SIZE);		// Gesamtlänge der Daten, mit Padding
	uint8_t bytesLeft = data_len;									// Länge der zu kopierenden Daten. Nur unterschiedlich wenn Länge < MIN_DATA_SIZE

#ifdef USE_MAC
	
	while (bytesLeft--)
		*ptrA++ = *data++;																						// Daten in den Ausgangspuffer kopieren
	
	// MAC über komplette Nachricht in place berechnen (bis auf den MAC selber und die CRC16):
	aes_mac_calculate(&mFtr->MAC[0], MAC_SIZE, &msgBuffer[0], sizeof(MessageHeader) + len);
	
	// Daten in place verschlüsseln:
	aes_cryptPayload(dataPtr, max(data_len, MIN_DATA_SIZE), mHdr->sensorAddress, useSequenceNumber, mHdr->flags & FLAG_A_TO_S);
	
#else
	printf("Create CRC32: ");
	mFtr->payloadCRC = CRC32START;								// Startwert für CRC
	// So lange wir Datenbytes haben..
	while (bytesLeft--) {
		len--;
		mFtr->payloadCRC = crc_calcCRC32r(mFtr->payloadCRC, *data);		// CRC updaten..
		printf("%.2x ", *data);
		*ptrA++ = *data++;																						// und in Ausgangspuffer kopieren
	}
	printf("\n");
	
	// Über die Nutzdaten hinaus gehende Bytes mit einrechnen (0-Padding)
	while (len--)
		mFtr->payloadCRC = crc_calcCRC32r(mFtr->payloadCRC, 0);
	
	// Daten in place verschlüsseln:
	aes_cryptPayload(dataPtr, max(data_len, MIN_DATA_SIZE)+CRC32LEN, mHdr->sensorAddress, useSequenceNumber, mHdr->flags & FLAG_A_TO_S);
#endif
	
	// Gesamt-Prüfsumme berechnen:
	mFtr->messageCRC = CRC16START;
	len = sizeof(msgBuffer)-2; //-2 für CRC16-Feld am Ende
	ptrA = &msgBuffer[0];
	printf("SEND: ");
	while (len--) {
		printf("%.2x ", *ptrA);
		mFtr->messageCRC = crc_calcCRC16r(mFtr->messageCRC, *ptrA++);
	}
	printf("/ %.2x ", *ptrA++);
	printf("%.2x (%d bytes)\n", *ptrA++, sizeof(msgBuffer));
	
	
	// Okay, Nachricht ist fertig zusammen gestellt. Senden:
	com_sendOutgoingData(&msgBuffer[0], sizeof(msgBuffer));
}

// Korrekt verschlüsselte, empfangene Nachrichten behandeln:
void com_messageReceived(MessageHeader * mHdr, uint8_t * payload, uint8_t payloadLength) {
	uint32_t * intSeqNum;
	
	if (currentRole) {
		com_println("Message received, role AGGREGATOR.\n");
		intSeqNum = &currentSequenceNumber;
	} else {
		com_println("Message received, role SENSOR.\n");
		intSeqNum = &simSeqNum;
	}
	
	if (mHdr->flags & FLAG_IS_ACK) {
		if (payload[0] || payload[1] || payload[2] || payload[3]) {
			com_println("Received wrong ACK.\n");
			return; // Kein gültiges ACK - da muss der Payload komplett \0 sein!
		}
		
		// ACK-SeqNums müssen == unAckedMessage sein
		if (mHdr->sequenceNumber != unAckedMessage) {
			com_println("Wrong ACK\n");
			printf("-> (%d recv vs %d expct.\n", mHdr->sequenceNumber, unAckedMessage);
			return;
		}
		
		// Okay, wunderbar. Sequenznummer ist größer/gleich der aktuellen Sequenznummer. Unsere Sequenznummer entsprechend anpassen:
		//*intSeqNum = mHdr->sequenceNumber;
		
		// ACK für ausstehende Nachricht:
		unAckedMessage = 0;
		
		com_messageAcked();
		
		com_println("VALID ACK!\n");
		
		// Perfekt.
	} else {
		// Eingehende, neue Nachricht:
		if (payload[0] == MESSAGE_TYPE_NACK_SEQNUM) {
			printf("NACK seqnum! Old %d vs new %d. Forwarding..\n", *intSeqNum, mHdr->sequenceNumber);
			// Semi-ACK, wir liegen mit der Sequenznummer zu weit zurück. Alte Nachricht muss wiederholt werden, allerdings mit neuer Sequenznummer:
			if (*intSeqNum < mHdr->sequenceNumber) {
				*intSeqNum = mHdr->sequenceNumber + 1;
			}
			unAckedMessage = 0;
			messageTimeout = 0;
			com_messageTimeout();
			
		} else {
			if (*intSeqNum < mHdr->sequenceNumber) {
				com_println("Received valid seqnum message. ACKing..\n");
				// ACK generieren, Nachricht empfangen:
				communication_sendCommand(mHdr->sensorAddress, 0, 0, mHdr->sequenceNumber, 1); // 1=Is ACK, ohne Daten und mit Länge 0
				// Anschließend die gültige Sequenznummer hoch zählen
				*intSeqNum = mHdr->sequenceNumber;
				// .. und Nachricht weiter leiten:
				com_processValidMessage(payload, payloadLength);
			} else {
				printf("Received low seqnum message. Rejecting..\n");
				// Versuch uns eine Nachricht mit zu kleiner Sequenznummer unter zu jubeln. Wir antworten mit 255er-Nachricht und aktuelle Sequenznummer + 1
				uint8_t byte255 = MESSAGE_TYPE_NACK_SEQNUM;
				communication_sendCommand(mHdr->sensorAddress, &byte255, 1, *intSeqNum + 1, 0); 
			}
		}
	}
}

#warning TODO: Was passiert wenn beide Seiten gleichzeitig eine Nachricht mit SeqNum+1 senden? Beide bekommen eine 255er-Nachricht zurück, beide versuchen
#warning       es damit erneut, beide bekommen wieder eine 255er usw.. Eigentlich müsste man doch irgendwie zwischen eigener und fremder SeqNum trennen..?



/*
 * Highlevel-Funktionen um Nachrichten zu senden und zu empfangen:
 *
 *
 *
*/

// Wir haben eine gültige Nachricht empfangen:
void com_processValidMessage(uint8_t * payload, uint8_t payloadLength) {
	printf("PROCESS VALID MESSAGE!\n");
	printf("Content: %.*s\n", payloadLength, payload);
	
	#warning TODO.
}

// Eine gesendete Nachricht wurde in einer gewissen Zeit nicht bestätigt:
void com_messageTimeout() {
	printf("MESSAGE TIMEOUT!\n");
	
	// Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload = (uint8_t*)"Das ist zwei Test.";
	com_sendMessage(payload, 18);	
	//#warning TODO.
}

#ifdef TESTING
void com_messageAcked() {
	// Nothing in here	
}
#endif

void com_sendOutgoingData(uint8_t * ptr, uint8_t length) {
	printf("Outgoing data of %d bytes.\n", length);
	// Wenn uns niemand die Daten abnimmt, schicken wir sie per Loopback wieder rein und simulieren die Gegenseite:
#ifdef TESTING
	// Für die Auswertung zum Testen müssen wir die Rolle wechseln:
	currentRole ^= 1;
	printf("Switching role to %s.\n", currentRole ? "AGGREGATOR" : "SENSOR");
	
	// Loopback:
	communication_dataIn(ptr, length);
	communication_poll();
	
	// Und zurück zur "normalen" Rolle:
	currentRole ^= 1;
	printf("Switching role back to %s.\n", currentRole ? "AGGREGATOR" : "SENSOR");
#endif
}



// Senden eines beliebigen Payloads:
uint8_t com_sendMessage(uint8_t * data, uint8_t len) {
	if (unAckedMessage)
		return 0;
	
	uint32_t * intSeqNum;
	if (currentRole) {
		printf("Sending, role AGGREGATOR.\n");
		intSeqNum = &currentSequenceNumber;
	} else {
		printf("Sending, role SENSOR.\n");
		intSeqNum = &simSeqNum;
	}
	
	unAckedMessage = ++(*intSeqNum);
	messageTimeout = com_getMillis();
	
	// Müssen wir so verschachtelt machen da die Funktion hier nicht vor dem Ende der Simulation zurückkehrt:
	communication_sendCommand(myAddress, data, len, *intSeqNum, 0); // 0=Das ist kein ACK.
	
	return 1;
}

uint32_t com_getMillis() {
	// Dummy
	static uint32_t myMillis;
	return myMillis++;	
}

#ifdef TESTING

int main() {
	uint16_t i;
	
	//uint8_t key[] = {0xab, 0xcd, 0xef, 0x91, 0x34, 0xef, 0xab, 0xcd, 0xef, 0x91, 0x34, 0xef, 0xab, 0xcd, 0xef, 0x91};
	//uint8_t key[] = {0x3d, 0xdc, 0x57, 0xcc, 0x89, 0x7c, 0xb0, 0x50, 0x6c, 0xd4, 0x1a, 0x89, 0x77, 0x65, 0x87, 0x56};
	//uint8_t key[] = {0x5b, 0x42, 0xc2, 0x82, 0xd1, 0x89, 0xc5, 0x7c, 0xe5, 0xea, 0x54, 0x80, 0x23, 0xa6, 0x92, 0x2c};
	uint8_t key[] = {0x73, 0x3a, 0xbf, 0xd6, 0x52, 0xf6, 0xa8, 0x85, 0x1f, 0x91, 0x35, 0xcc, 0xb5, 0x97, 0x17, 0x49};
	
	aes_init(&key[0], sizeof(key));
	
	// Aufsetzen:
	communication_init(0x11223344);
	//communication_init(0x44332211);
	
	currentRole = ROLE_SENSOR;
	
	// Nachricht mit Sequenznummer 123 senden:
	
	currentSequenceNumber = simSeqNum = 15140;
	
	uint8_t * payload = "\0\0\0\0\0\0\0\0";
	
	com_sendMessage(payload, 8);
	
	printf("\n\nSTEP 2\n\n\n");
	
	// Noch eine Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload2 = "Das ist ein weiterer Test.";
	com_sendMessage(payload2, 26);
	
	// Rolle wechseln und Nachricht zurück schicken:
	printf("\n\nSTEP 3\n\n\n");
	
	
	currentRole = ROLE_AGGREGATOR;
	// Noch eine Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload3 = "Vielen Dank dafür!";
	com_sendMessage(payload3, 18);
	
	for (i=0;i<3000;i++) {
		communication_poll();
	}
	
	printf("Nachrichtensimulation:\n");

	currentRole = ROLE_AGGREGATOR;
	currentSequenceNumber = simSeqNum = 0;
	
	printf("\n\nMAC\n");
	
	uint8_t data[] = {0xf9, 0x00, 0x0c, 0x2e, 0x76, 0xb2, 0xb6, 0x29, 0x01, 0x00, 0x00, 0xc5, 0xa1, 0x77, 0x94, 0xab, 0xaf, 0x9d, 0x01, 0xbd, 0xd8, 0x79, 0x29};
	uint8_t output[16];
	aes_mac_calculate(&output[0], 8, &data[0], sizeof(data));




	
	printf("\n\nEingang falsches ACK:\n\n");
	
	unAckedMessage = 0;						// Welche Sequenznummer hat die unbestätigte Nachricht die noch unterwegs ist?
	messageTimeout = com_getMillis() - MESSAGE_ACK_TIMEOUT + 45;		// Timeout für eine unbestätigte Nachricht
	currentSequenceNumber = 0;		// Aktuelle Sequenznummer
	simSeqNum = 0;								// Simulierte Sequenznummer des Gegenübers
	myAddress = 0xb1f4e3ab;
	
	
	currentRole = ROLE_SENSOR;
	
	//uint8_t tdata[] = {0xf9, 0x30, 0x04, 0xcd, 0x8a, 0x72, 0xc9, 0xe2, 0x52, 0x00, 0x00, 0x4b, 0xf6, 0x3c, 0xa5, 0xbb, 0x28, 0x78, 0x31, 0x99, 0x6b};
	//	uint8_t tdata[] = {0xf9, 0x10, 0x04, 0xab, 0xe3, 0xf4, 0xb1, 0x22, 0x00, 0x00, 0x00, 0xef, 0xcb, 0x3c, 0x0c, 0x0b, 0x18, 0xc1, 0xbc, 0x58, 0x15, 0xd3, 0xa8, 0xa3, 0x0a};
	// {0xf9, 0x10, 0x04, 0xab, 0xe3, 0xf4, 0xb1, 0x22, 0x00, 0x00, 0x00, 0xef, 0xcb, 0x3c, 0x0c, 0xc0, 0xbb, 0x18, 0xc1, 0xbc, 0x58, 0x15, 0xd3, 0xa8, 0xa3, 0x0a}
	//uint8_t tdata[] = {0xf9, 0x00, 0x13, 0xab, 0xe3, 0xf4, 0xb1, 0x02, 0x00, 0x00, 0x00, 0x14, 0x3b, 0x2e, 0xd9, 0x3d, 0xf3, 0x6e, 0xf6, 0x1c, 0x03, 0xfd, 0xde, 0xf1, 0xe8, 0x4e, 0xa5, 0x1a, 0xf3, 0x2a, 0x55, 0x86, 0xbb, 0xe8, 0xe2, 0x2f, 0x0d, 0x59, 0x22, 0x15};
	uint8_t tdata[] = {0xf9 ,0x30 ,0x4 ,0xab ,0xe3 ,0xf4 ,0xb1 ,0x60 ,0x0 ,0x0 ,0x0 ,0xdd ,0x94 ,0xcb ,0x7c ,0x20 ,0x91 ,0x20 ,0x92 ,0x5c ,0x95 ,0xfc ,0xff ,0x91 ,0x0a};
	
	
	communication_dataIn(&tdata[0], sizeof(tdata));
	
	uint8_t j;
	for (j=0;j<50;j++)
		communication_poll();
	
	
	//uint8_t testdata[] = {0xF9, 0x00, 0x0C, 0x44, 0x33, 0x22, 0x11, 0x3E, 0x00, 0x00, 0x00, 0x70, 0x5E, 0x1C, 0x2F, 0xA9, 0xD2, 0x64, 0x54, 0x35, 0x81, 0x8D, 0xD9, 0x76, 0x26, 0x14, 0x83, 0xAF, 0x67 };
	//communication_dataIn(&testdata[0], sizeof(testdata));
	//communication_poll();
	
}

void com_println(char * msg) {
	printf(msg);
}

#endif
