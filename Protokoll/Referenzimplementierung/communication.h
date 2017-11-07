#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_



#include <stdint.h>

typedef struct __attribute__((packed)) {
	uint8_t sync;
	uint8_t flags;
	uint8_t payloadLength;
	uint32_t sensorAddress;
	uint32_t sequenceNumber;
} MessageHeader;

typedef struct __attribute__((packed)) {
	uint32_t payloadCRC;
	uint16_t messageCRC;
} MessageFooter;

#define SYNCBYTE									249
#define BITS_VERSION_0						0b00
#define BIT_A_TO_S								1
#define BIT_S_TO_A								0
#define BIT_IS_ACK								1
#define FLAG_IS_ACK								0b100000
#define FLAG_A_TO_S								0b010000
#define RF_UART_IN_TIMEOUT_MS			25 // Darf nicht viel höher sein. Sonst wird die Kommunikation potentiell zu lange blockiert
#define MIN_DATA_SIZE							4
#define MESSAGE_ACK_TIMEOUT				1000
#define MESSAGE_TYPE_NACK_SEQNUM	0xff
#define CRC16MASK									0xa001
#define CRC32MASK									0x04c11db7
#define CRC16START								0xffff
#define CRC32START								0xffffffff



// Initialisierung:
void communication_init(uint32_t sensorAddress);

// Empfangenen Datenblock füttern:
void communication_dataIn(uint8_t * dataPtr, uint8_t len);

// Diese Daten sollen gesendet werden - diese Funktion muss extern implementiert werden:
void com_sendOutgoingData(uint8_t * ptr, uint8_t length);

// Regelmäßig nachgucken ob Daten da sind bzw Timeouts zählen. Sollte jede Millisekunde aufgerufen werden (ansonsten die Timeout-Werte anpassen)
void communication_poll(void);

// Wir haben eine gültige Nachricht empfangen - diese Funktion muss extern implementiert werden:
void com_processValidMessage(uint8_t * payload, uint8_t payloadLength);

// Die Nachricht ist verloren gegangen/die Sequenznummer musste erst abgeglichen werden - diese Funktion muss extern implementiert werden:
void com_messageTimeout();

// Datenpaket senden:
uint8_t com_sendMessage(uint8_t * data, uint8_t len);

// Systemzeit in Millisekunden abfragen:
uint32_t com_getMillis();

#endif
