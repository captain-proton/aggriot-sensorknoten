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

#define RF_UART_IN_TIMEOUT_MS		25 // Darf nicht viel höher sein. Sonst wird die Kommunikation potentiell zu lange blockiert

#define MIN_DATA_SIZE			4

static uint8_t executingCOMCheck = 0;
RB_BUFFER_t * inBuffer;
static uint8_t in_timeout_ms = 0;



void communication_init() {
	static uint8_t inBuffer_space[256];
	// Puffer initialisieren:
	inBuffer = rb_createBuffer(&inBuffer_space[0], sizeof(inBuffer_space));
}

void communication_dataIn(uint8_t * dataPtr, uint8_t len) {
	while (len--)
		rb_put(inBuffer, *dataPtr++);
}

void communication_checkIncoming(void) { // Funktion wird regelmäßig (möglichst exakt jede Millisekunde) aufgerufen
	uint16_t rxc;
	uint8_t cmdlen;
	uint8_t i;
	uint8_t p;
	
	if (executingCOMCheck)
		return;
	executingCOMCheck = 1;
	
	// Puffer überprüfen:
	while ((rxc = rb_getCount_sync(inBuffer))) {
		if (SYNCBYTE == rb_peek_sync(inBuffer, 0)) {
			// Danach kommt die Länge der eigentlichen Information. Falls die Angabe hier < 4 ist,
			// ist der Payload 4 Byte lang aber nur ein Teil davon ist relevant für den Nutzer.
			if (rxc > 1) {
				
				// Längeninformation ist zweites Byte:
				cmdlen = rb_peek_sync(inBuffer, 1);
				uint16_t packetLength = (uint16_t)cmdlen + max(cmdlen, 4) + sizeof(MessageHeader) + sizeof(MessageFooter); // +1 Sync-Byte
				
				if (rxc >= packetLength) {
					//if ((uint8_t)(cmdlen + 4) <= rxc) { // Die 4 Bytes SYNCBYTE, LÄNGE, CRCHIGH und CRCLOW noch einrechnen
						setDebugStep(120);
						
						// Zuerst die komplette CRC berechnen. Wenn wir einem unechten SYNC aufgesessen sind,
						// sollte das verhindern, das zu viele der echten Zeichen weggeworfen werden.
						// Auf diese Art kann der dahinter kommende, "echte" Befehl noch ausgewertet werden:
						mycrc = crc_calcCRC16r(0xffff, cmdlen);
						
						// CRC berechnen. Ab Startbyte, ohne die eigentliche CRC16-Prüfsumme am Ende:
						for (i=0;i<packetLength-2;i++)
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
							
							// Daten aus dem Eingangspuffer löschen:
							rb_delete_sync(inBuffer, packetLength);
							
							// Prüfen ob wir der Empfänger sind:
							if ((mHdr.flags >> 6) == BITS_VERSION_0) {
								if (mHdr.flags & FLAG_A_TO_S) {
									// Eingehende Nachricht zu einem Sensor. Sind wir gemeint?
									if (mHdr.sensorAddress == MY_SENSOR_ADDRESS) {
										
										// Versuchen die Nachricht zu entschlüsseln:
										communication_cryptPayload(&messageData[0], sizeof(messageData), mHdr.sequenceNumber, 1); // 1 = Incoming message
										
										uint32_t calculatedCRC = 0xffffffff;
										// Testen, ob die Prüfsumme jetzt zu den entschlüsselten Daten passt:
										for (uint8_t i=0;i<sizeof(messageData);i++) {
											calculatedCRC = crc_calcCRC32(calculatedCRC, messageData[i]);
										}
										
										// Ist korrekt ver/entschlüsselt (und übertragen) worden?
										if (calculatedCRC == mFtr.payloadCRC) {
											// Nachricht verarbeiten:
											com_messageReceived(&mHdr, &messageData[0], cmdlen);
											
										} // .. nicht korrekt verschlüsselt: Nachricht ignorieren.
										
									} // .. Nicht für uns. Ignorieren.
									
								} // .. Nachricht an den Aggregator. Ignorieren.
								
							} // .. Andere Protokollversion. Die kennen wir nicht. Ignorieren.
							
						} else {
							
							// Die CRC war falsch. Eventuell war das garkein Befehl auf den wir reagiert haben!
							// Also einfach das "falsche" SYNC wegwerfen und nach einem neuen SYNC suchen:
							rb_delete_sync(inBuffer, 1);
							
						}
						
						// Wir haben irgendetwas aus dem Eingangspuffer gelöscht - es könnte also noch ein Befehl vorhanden sein: Nächsten Befehl abarbeiten..
						continue;
						
					}
					
				} else {
					
					// Das gedachte Sync-Zeichen war kein Sync! Es sind aber eventuell noch weitere Daten im Eingangspuffer..
					rb_delete_sync(inBuffer, 1);
					continue;
					
				}
				
			}
			
			// Der Befehl ist noch nicht vollständig. Wenn aber die einzelnen Zeichen eines Befehls mehr als (X)ms auseinander liegen,
			// wird der Befehl abgebrochen da wir evtl ein nicht-sync-Zeichen als SYNC interpretiert haben weil der Sensor
			// den Anfang des echten Befehls nicht mitbekommen hat. In dem Fall übergehen wir das SYNC-Zeichen und laufen direkt zum
			// nächsten SYNC-Byte.
			if (!in_timeout_ms) {
				// SYNC wegwerfen:
				rb_delete_sync(inBuffer, 1);
				continue; // Und gleich beim nächsten Byte gucken ob das ein SYNC-Byte sein könnte..
			} else {
				// Wir laufen einmal pro Millisekunde. Perfekt also um den Zähler runter zu zählen wenn das nötig ist.
#warning TODO: Anpassen an Arduino?
				in_timeout_ms--; // Tick, tick..
				// Timeout für ACKs:
				if (unAckedMessage) {
					if (!messageTimeout--) {
						unAckedMessage = 0;
						com_messageTimeout();
					}
				}
				
				break;
			}
		} else {
			// Zeichen wird verworfen:
			rb_delete_sync(inBuffer, 1);
		}
	}
	
	executingCOMCheck = 0;
}

void communication_sendCommand(uint8_t* data, uint8_t data_len, uin32_t useSequenceNumber, uint8_t isAck) {
	uint8_t msgBuffer[max(data_len, MIN_DATA_SIZE) + sizeof(MessageHeader) + sizeof(MessageFooter)];
	MessageHeader * mHdr = (MessageHeader*)&msgBuffer[0];
	MessageFooter * mFtr = (MessageFooter*)&msgBuffer[sizeof(MessageHeader) + max(data_len, MIN_DATA_SIZE)];
	uint8_t * dataPtr = &msgBuffer[sizeof(MessageHeader)];
	
	// Wir senden mindestens 4 Bytes und füllen unbenutzte Bytes mit \0:
	msgBuffer[sizeof(MessageHeader)+0] = 0;
	msgBuffer[sizeof(MessageHeader)+1] = 0;
	msgBuffer[sizeof(MessageHeader)+2] = 0;
	msgBuffer[sizeof(MessageHeader)+3] = 0;
	
	mHdr->sync = SYNCBYTE;
	mHdr->flags = isAck ? FLAG_IS_ACK : 0;
	mHdr->payloadLength = data_len;
	mHdr->sensorAddress = MY_SENSOR_ADDRESS;
	mHdr->sequenceNumber = useSequenceNumber;
	
	// So viele Daten kopieren wie wir haben, gleichzeitig die Pre-Crypt-Prüfsumme berechnen:
	uint8_t * ptrA = dataPtr;
	uint8_t len = data_len;
	mFtr.payloadCRC = 0xffffffff;
	while (len--) {
		mFtr.payloadCRC = crc_calcCRC32(mFtr.payloadCRC, *data);
		*ptrA++ = *data++;
	}
	
	// Daten verschlüsseln:
	communication_cryptPayload(dataPtr, max(data_len, MIN_DATA_SIZE), useSequenceNumber, 0);
	
	// Gesamt-Prüfsumme berechnen:
	mFtr.messageCRC = 0xffff;
	len = sizeof(msgBuffer)-2; //-2 für CRC16-Feld am Ende
	ptrA = &msgBuffer[0];
	while (len--) {
		mFtr.messageCRC = crc_calcCRC16r(mFtr.messageCRC, *ptrA);
	}
	
	// Okay, Nachricht ist fertig zusammen gestellt. Senden:
	RH_RF95.send(&msgBuffer[0], sizeof(msgBuffer));
}

// Verschlüsseln:
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
		
#warning TODO: Eventuell Big Endian einfügen? Je nachdem wie die andere Seite es implementiert
		// Ursprungsdaten einfüllen
		block.sensorAddress = MY_SENSOR_ADDRESS;
		block.sequenceNumber = sequenceNumber;
		block.blockNumber = blockNumber;
		
		// Verschlüsseln:
		aes_encryptBlock(&block);
		
		do {
			if (!payloadLength--)
				return;
			*payload ^= block.byteblock[bufferPosition & 15];
		} while ((++bufferPosition & 15));
	} while (++blockNumber);
}



void com_messageReceived(MessageHeader * mHdr, uint8_t * payload, uint8_t payloadLength) {
	if (mHdr->flags & FLAG_IS_ACK) {
		if (payload[0] || payload[1] || payload[2] || payload[3]) {
			return; // Kein gültiges ACK - da muss der Payload komplett \0 sein!
		
		// ACKs müssen SeqNum >= aktuelle Sequenznummer haben
		if (mHdr->sequenceNumber < currentSequenceNumber)
			return;
		
		// Okay, wunderbar. Sequenznummer ist größer/gleich der aktuellen Sequenznummer. Unsere Sequenznummer entsprechend anpassen:
		currentSequenceNumber = mHdr->sequenceNumber;
		
		// ACK für ausstehende Nachricht:
		unAckedMessage = 0;
		
		// Perfekt.
	} else {
		// Eingehende, neue Nachricht:
		if (payload[0] == 255) {
			// Semi-ACK, wir liegen mit der Sequenznummer zu weit zurück. Alte Nachricht muss wiederholt werden, allerdings mit neuer Sequenznummer:
			if (currentSequenceNumber < mHdr->sequenceNumber) {
				currentSequenceNumber = mHdr->sequenceNumber + 1;
			}
#warning TODO: Hier könnte man die ursprüngliche Nachricht mit der zu niedrigen Sequenznummer direkt wiederholen ohne auf das ACK-Timeout zu warten..			
		} else {
			if (currentSequenceNumber < mHdr->sequenceNumber) {
				// ACK generieren, Nachricht empfangen:
				communication_sendCommand(0, 0, mHdr->sequenceNumber, 1); // 1=Is ACK, ohne Daten und mit Länge 0
				// Anschließend die gültige Sequenznummer hoch zählen, +1 (wir müssen beim nächsten Mal die nächste gültige Sequenznummer benutzen)
				currentSequenceNumber = mHdr->sequenceNumber + 1;
			} else {
				// Versuch uns eine Nachricht mit zu kleiner Sequenznummer unter zu jubeln. Wir antworten mit 255er-Nachricht und aktuelle Sequenznummer + 1
				uint8_t byte255 = 255;
				communication_sendCommand(&byte255, 1, currentSequenceNumber + 1, 0); 
			}
		}
	}
}

#warning TODO: Was passiert wenn beide Seiten gleichzeitig eine Nachricht mit SeqNum+1 senden? Beide bekommen eine 255er-Nachricht zurück, beide versuchen
#warning       es damit erneut, beide bekommen wieder eine 255er usw.. Eigentlich müsste man doch irgendwie zwischen eigener und fremder SeqNum trennen..?

uint8_t com_sendMessage(uint8_t * data, uint8_t len) {
	if (unAckedMessage)
		return 0;
	
	communication_sendCommand(data, len, currentSequenceNumber, 0); // 0=Das ist kein ACK.
	unAckedMessage = 1;
	messageTimeout = 1000; // 1000ms?
}

void com_messageTimeout() {
	#warning TODO.
}

