/*
 * aes.c
 *
 * Created: 27.10.2017 18:01:45
 *  Author: thagemeier
 */ 
#ifndef _AES_H_
#define _AES_H_

/*
 * aes.c
 *
 * Created: 27.10.2017 18:01:39
 *  Author: thagemeier
 */ 
#include <stdint.h>

void aes_init(const uint8_t * key, uint8_t len);
void aes_setKey(const uint8_t* thiskey, uint8_t len);
void aes_cryptPayload(uint8_t * payload, uint8_t payloadLength, uint32_t sensorAddress, uint32_t sequenceNumber, uint8_t isIncoming);

#define EXPANDED_KEY_SIZE		176
#define KEY_SIZE						16
#define BLOCK_SIZE					16

#endif
