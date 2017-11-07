#include <avr/io.h>
#include <stdio.h>
#include "aes.h"
#include "communication.h"
#include "ringBuffer.h"

#define max(x, y)							((x) > (y) ? (x) : (y))

uint16_t crc_calcCRC16r(uint16_t crc, uint8_t c) {
  unsigned char i;
  for(i=0;i<8;i++) {
    if ((crc ^ c) & 1) {
    	crc = (crc>>1) ^ CRC16MASK;
    } else {
    	crc	>>=	1;
    }
    c	>>=	1;
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
    c	>>=	1;
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
static uint8_t executingCOMCheck = 0;		// Wird communication_poll gerade ausgef�hrt?
static uint8_t in_timeout_ms = 0; 			// Timeout f�r eingehende Bytes
static uint32_t unAckedMessage;					// Welche Sequenznummer hat die unbest�tigte Nachricht die noch unterwegs ist?
static uint16_t messageTimeout;					// Timeout f�r eine unbest�tigte Nachricht
static uint32_t currentSequenceNumber;	// Aktuelle Sequenznummer
static uint32_t simSeqNum;							// Simulierte Sequenznummer des Gegen�bers
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
void com_messageAcked() __attribute__((weak));
void com_sendOutgoingData(uint8_t * ptr, uint8_t length) __attribute__((weak));
#endif

void communication_init(uint32_t sensorAddress) {
	static uint8_t inBuffer_space[256];
	inBuffer = rb_createBuffer(&inBuffer_space[0], sizeof(inBuffer_space));
	if (inBuffer)
		printf("Input buffer created.\n");
	myAddress = sensorAddress;
}

// Empfangenen Datenblock f�ttern:
void communication_dataIn(uint8_t * dataPtr, uint8_t len) {
	printf("Received %d bytes..\n", len);
	while (len--)
		rb_put(inBuffer, *dataPtr++);
}

// Regelm��ig nachgucken ob Daten da sind bzw Timeouts z�hlen
void communication_poll(void) { // Funktion wird regelm��ig (m�glichst exakt jede Millisekunde) aufgerufen
	uint16_t rxc;
	uint8_t cmdlen;
	uint8_t i;
	uint16_t mycrc;
	
#ifndef TESTING
	if (executingCOMCheck)
		return;
	executingCOMCheck = 1;
#endif
	
	// Puffer �berpr�fen:
	while ((rxc = rb_getCount_sync(inBuffer))) {
		if (SYNCBYTE == rb_peek_sync(inBuffer, 0)) {
			
			// Danach kommt die L�nge der eigentlichen Information. Falls die Angabe hier < 4 ist,
			// ist der Payload 4 Byte lang aber nur ein Teil davon ist relevant f�r den Nutzer.
			if (rxc > 1) {
				
				// L�ngeninformation ist drittes Byte, nach den Flags:
				cmdlen = rb_peek_sync(inBuffer, 2);
				uint16_t packetLength = max(cmdlen, 4) + sizeof(MessageHeader) + sizeof(MessageFooter);
				
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
					
					// CRC berechnen. Ab Startbyte, ohne die eigentliche CRC16-Pr�fsumme am Ende:
					printf("RECV: ");
					for (i=0;i<packetLength-2;i++) {
						printf("%.2x ", rb_peek_sync(inBuffer, i));
						mycrc = crc_calcCRC16r(mycrc, rb_peek_sync(inBuffer, i));
					}
					printf("\n");
					
					printf("Calculated CRC %x ", mycrc);
					
					if (((mycrc >> 8) == rb_peek_sync(inBuffer, packetLength-1)) && (((uint8_t)mycrc) == rb_peek_sync(inBuffer, packetLength-2))) {
						printf("is valid.\n");
						
						// Korrekt empfangen. Paketdaten in die Headerstrukturen kopieren:
						MessageHeader mHdr;
						MessageFooter mFtr;
						uint8_t messageData[max(cmdlen, 4)];
						uint8_t len = sizeof(MessageHeader);
						uint16_t p = 0;
						uint8_t * ptr = (uint8_t*)&mHdr;
						
						// Daten im Header kopieren:
						while (len--)
							*ptr++ = rb_peek_sync(inBuffer, p++);
						
						// Daten selbst kopieren:
						len = sizeof(messageData);
						ptr = &messageData[0];
						while (len--)
							*ptr++ = rb_peek_sync(inBuffer, p++);
						
						// Daten im Footer kopieren:
						ptr = (uint8_t*)&mFtr;
						len = sizeof(MessageFooter);
						while (len--)
							*ptr++ = rb_peek_sync(inBuffer, p++);
						
						// Daten aus dem Eingangspuffer l�schen:
						rb_delete_sync(inBuffer, packetLength);
						
						// Pr�fen ob wir der Empf�nger sind:
						if ((mHdr.flags >> 6) == BITS_VERSION_0) {
							if (((mHdr.flags & FLAG_A_TO_S) ? ROLE_SENSOR : ROLE_AGGREGATOR) == currentRole) {
								
								printf("-- Incoming message to sensor 0x%x --\n", mHdr.sensorAddress);
								
								
								// Eingehende Nachricht zu einem Sensor/Aggregator. Sind wir gemeint?
								if ((mHdr.sensorAddress == myAddress) || (currentRole == ROLE_AGGREGATOR)) {
									printf("   That's me!\n");
									
									// Versuchen die Nachricht zu entschl�sseln:
									aes_cryptPayload(&messageData[0], sizeof(messageData), mHdr.sensorAddress, mHdr.sequenceNumber, mHdr.flags & FLAG_A_TO_S); // 1 = Incoming message
									
									uint32_t calculatedCRC = CRC32START;
									// Testen, ob die Pr�fsumme jetzt zu den entschl�sselten Daten passt:
									printf(":: CRC32 over %d bytes.\n", sizeof(messageData));
									for (i=0;i<sizeof(messageData);i++) {
										calculatedCRC = crc_calcCRC32r(calculatedCRC, messageData[i]);
									}
									
									// Ist korrekt ver/entschl�sselt (und �bertragen) worden?
									if (calculatedCRC == mFtr.payloadCRC) {
										printf("   Valid encryption.\n");
										// Nachricht verarbeiten:
										com_messageReceived(&mHdr, &messageData[0], cmdlen);
										
									} else { // .. nicht korrekt verschl�sselt: Nachricht ignorieren.
										printf("   Invalid encryption. Dropping message.\n");
									}
								} else { // .. Nicht f�r uns. Ignorieren.
									printf("   Not for me.");
								}
							} else { // .. Nachricht an den Aggregator. Ignorieren.
								printf("Upstream/downstream wrong!\n");
							}
						} else { // .. Andere Protokollversion. Die kennen wir nicht. Ignorieren.
							printf("Wrong version.\n");
						}
						
					} else {
						
						// Die CRC war falsch. Eventuell war das garkein Befehl auf den wir reagiert haben!
						// Also einfach das "falsche" SYNC wegwerfen und nach einem neuen SYNC suchen:
						rb_delete_sync(inBuffer, 1);
						
						printf("is invalid.\n");
						
					}
					
					// Wir haben irgendetwas aus dem Eingangspuffer gel�scht - es k�nnte also noch ein Befehl vorhanden sein: N�chsten Befehl abarbeiten..
					continue;
					
				} else {
					
					//// Das gedachte Sync-Zeichen war kein Sync! Es sind aber eventuell noch weitere Daten im Eingangspuffer..
					//rb_delete_sync(inBuffer, 1);
					//continue;
					
				}
				
			}
			
			// Der Befehl ist noch nicht vollst�ndig. Wenn aber die einzelnen Zeichen eines Befehls mehr als (X)ms auseinander liegen,
			// wird der Befehl abgebrochen da wir evtl ein nicht-sync-Zeichen als SYNC interpretiert haben weil der Sensor
			// den Anfang des echten Befehls nicht mitbekommen hat. In dem Fall �bergehen wir das SYNC-Zeichen und laufen direkt zum
			// n�chsten SYNC-Byte.
			if (!in_timeout_ms) {
				printf("Byte timeout. Dropping start byte.\n");
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
	
	// Timeout f�r erwartete ACKs:
	if (unAckedMessage) {
		if (!messageTimeout--) {
			unAckedMessage = 0;
			messageTimeout = 0;
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
	
	// Wir senden mindestens 4 Bytes und f�llen unbenutzte Bytes mit \0:
	dataPtr[0] = 0;
	dataPtr[1] = 0;
	dataPtr[2] = 0;
	dataPtr[3] = 0;
	
	mHdr->sync = SYNCBYTE;
	mHdr->flags = (isAck ? FLAG_IS_ACK : 0) | (currentRole == ROLE_AGGREGATOR ? FLAG_A_TO_S : 0)  ;
	mHdr->payloadLength = data_len;
	mHdr->sensorAddress = nodeAddress;
	mHdr->sequenceNumber = useSequenceNumber;
	
	// So viele Daten kopieren wie wir haben, gleichzeitig die Pre-Crypt-Pr�fsumme berechnen:
	uint8_t * ptrA = dataPtr;
	uint8_t len = max(data_len, MIN_DATA_SIZE);		// Gesamtl�nge der Daten, mit Padding
	uint8_t bytesLeft = data_len;									// L�nge der zu kopierenden Daten. Nur unterschiedlich wenn L�nge < MIN_DATA_SIZE
	mFtr->payloadCRC = CRC32START;								// Startwert f�r CRC
	// So lange wir Datenbytes haben..
	while (bytesLeft--) {
		len--;
		mFtr->payloadCRC = crc_calcCRC32r(mFtr->payloadCRC, *data);		// CRC updaten..
		*ptrA++ = *data++;																						// und in Ausgangspuffer kopieren
	}
	// �ber die Nutzdaten hinaus gehende Bytes mit einrechnen (0-Padding)
	while (len--)
		mFtr->payloadCRC = crc_calcCRC32r(mFtr->payloadCRC, 0);
	
	// Daten in place verschl�sseln:
	aes_cryptPayload(dataPtr, max(data_len, MIN_DATA_SIZE), mHdr->sensorAddress, useSequenceNumber, mHdr->flags & FLAG_A_TO_S);
	
	// Gesamt-Pr�fsumme berechnen:
	mFtr->messageCRC = CRC16START;
	len = sizeof(msgBuffer)-2; //-2 f�r CRC16-Feld am Ende
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

// Korrekt verschl�sselte, empfangene Nachrichten behandeln:
void com_messageReceived(MessageHeader * mHdr, uint8_t * payload, uint8_t payloadLength) {
	uint32_t * intSeqNum;
	
	if (currentRole) {
		printf("Message received, role AGGREGATOR.\n");
		intSeqNum = &currentSequenceNumber;
	} else {
		printf("Message received, role SENSOR.\n");
		intSeqNum = &simSeqNum;
	}
	
	if (mHdr->flags & FLAG_IS_ACK) {
		if (payload[0] || payload[1] || payload[2] || payload[3]) {
			printf("Received wrong ACK.\n");
			return; // Kein g�ltiges ACK - da muss der Payload komplett \0 sein!
		}
		
		// ACK-SeqNums m�ssen == unAckedMessage sein
		if (mHdr->sequenceNumber != unAckedMessage) {
			printf("Wrong ACK (%d recv vs %d expct.\n", mHdr->sequenceNumber, unAckedMessage);
			return;
		}
		
		// Okay, wunderbar. Sequenznummer ist gr��er/gleich der aktuellen Sequenznummer. Unsere Sequenznummer entsprechend anpassen:
		//*intSeqNum = mHdr->sequenceNumber;
		
		// ACK f�r ausstehende Nachricht:
		unAckedMessage = 0;
		
		com_messageAcked();
		
		printf("VALID ACK!\n");
		
		// Perfekt.
	} else {
		// Eingehende, neue Nachricht:
		if (payload[0] == MESSAGE_TYPE_NACK_SEQNUM) {
			printf("NACK seqnum! Old %d vs new %d. Forwarding..\n", *intSeqNum, mHdr->sequenceNumber);
			// Semi-ACK, wir liegen mit der Sequenznummer zu weit zur�ck. Alte Nachricht muss wiederholt werden, allerdings mit neuer Sequenznummer:
			if (*intSeqNum < mHdr->sequenceNumber) {
				*intSeqNum = mHdr->sequenceNumber + 1;
			}
			unAckedMessage = 0;
			messageTimeout = 0;
			com_messageTimeout();
			
		} else {
			if (*intSeqNum < mHdr->sequenceNumber) {
				printf("Received valid seqnum message. ACKing..\n");
				// ACK generieren, Nachricht empfangen:
				communication_sendCommand(mHdr->sensorAddress, 0, 0, mHdr->sequenceNumber, 1); // 1=Is ACK, ohne Daten und mit L�nge 0
				// Anschlie�end die g�ltige Sequenznummer hoch z�hlen
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

#warning TODO: Was passiert wenn beide Seiten gleichzeitig eine Nachricht mit SeqNum+1 senden? Beide bekommen eine 255er-Nachricht zur�ck, beide versuchen
#warning       es damit erneut, beide bekommen wieder eine 255er usw.. Eigentlich m�sste man doch irgendwie zwischen eigener und fremder SeqNum trennen..?



/*
 * Highlevel-Funktionen um Nachrichten zu senden und zu empfangen:
 *
 *
 *
*/

// Wir haben eine g�ltige Nachricht empfangen:
void com_processValidMessage(uint8_t * payload, uint8_t payloadLength) {
	printf("PROCESS VALID MESSAGE!\n");
	printf("Content: %.*s\n", payloadLength, payload);
	
	#warning TODO.
}

// Eine gesendete Nachricht wurde in einer gewissen Zeit nicht best�tigt:
void com_messageTimeout() {
	printf("MESSAGE TIMEOUT!\n");
	
	// Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload = (uint8_t*)"Das ist zwei Test.";
	com_sendMessage(payload, 18);	
	//#warning TODO.
}

void com_messageAcked() {
	// Nothing in here	
}

void com_sendOutgoingData(uint8_t * ptr, uint8_t length) {
	printf("Outgoing data of %d bytes.\n", length);
	// Wenn uns niemand die Daten abnimmt, schicken wir sie per Loopback wieder rein und simulieren die Gegenseite:
#ifdef TESTING
	// F�r die Auswertung zum Testen m�ssen wir die Rolle wechseln:
	currentRole ^= 1;
	printf("Switching role to %s.\n", currentRole ? "AGGREGATOR" : "SENSOR");
	
	// Loopback:
	communication_dataIn(ptr, length);
	communication_poll();
	
	// Und zur�ck zur "normalen" Rolle:
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
	messageTimeout = MESSAGE_ACK_TIMEOUT;
	
	// M�ssen wir so verschachtelt machen da die Funktion hier nicht vor dem Ende der Simulation zur�ckkehrt:
	communication_sendCommand(myAddress, data, len, *intSeqNum, 0); // 0=Das ist kein ACK.
	
	return 1;
}

#ifdef TESTING

int main() {
	uint16_t i;
	
	// Aufsetzen:
	communication_init(0x11223344);
	
	currentRole = ROLE_SENSOR;
	
	// Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload = "Das ist ein Test.";
	com_sendMessage(payload, 17);
	
	printf("\n\nSTEP 2\n\n\n");
	
	// Noch eine Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload2 = "Das ist ein weiterer Test.";
	com_sendMessage(payload2, 26);
	
	// Rolle wechseln und Nachricht zur�ck schicken:
	printf("\n\nSTEP 3\n\n\n");
	
	
	currentRole = ROLE_AGGREGATOR;
	// Noch eine Nachricht mit Sequenznummer 123 senden:
	uint8_t * payload3 = "Vielen Dank daf�r!";
	com_sendMessage(payload3, 18);
	
	for (i=0;i<3000;i++) {
		communication_poll();
	}
}

#endif
