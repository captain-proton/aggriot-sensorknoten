#ifndef _COMMUNICATION_H_
#define _COMMUNICATION_H_



#include <stdint.h>

typedef struct {
	uint8_t sync;
	uint8_t flags;
	uint8_t payloadLength;
	uint32_t sensorAddress;
	uint32_t sequenceNumber;
} MessageHeader;

typedef struct {
	uint32_t payloadCRC;
	uint16_t messageCRC;
} MessageFooter;



#endif