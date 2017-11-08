/*
 * aes.c
 *
 * Created: 27.10.2017 18:01:39
 *  Author: thagemeier
 */ 
#ifndef TESTING
#include <avr/io.h>
#else
#include "aes_c.c" // Dirty but works

uint8_t * keyPtr;
#endif
#include "aes.h"

// These functions are provided by the assembler file. They should not be accessed from anywhere else but here:

void aes_encrypt(uint8_t* text16, uint8_t* expandedkey176);
#ifdef NEED_DECRYPTION_FEATURE
void aes_decrypt(uint8_t* text16, uint8_t* expandedkey176);
#endif
void aes_expand_key(uint8_t* key, uint8_t* expandedkeybuf176);
void aes_patch_key(uint8_t* key);

uint8_t expandedEncryptionKey[EXPANDED_KEY_SIZE];
#ifdef NEED_DECRYPTION_FEATURE
uint8_t expandedDecryptionKey[EXPANDED_KEY_SIZE];
#endif

void aes_setKey(const uint8_t* thiskey, uint8_t len) {
	// The key that is passed to this function is buffered in the AESkeyBuffer
	// so if the same key is passed again the expanded keys don't have to be
	// regenerated.
	if (len != BLOCK_SIZE)
		return;
#ifdef _AVR_IO_H_
	aes_expand_key((uint8_t*)thiskey, &expandedEncryptionKey[0]);
#else
	uint8_t i;
	printf("Encrypting on AVR with KEY: ");
	for (i=0;i<BLOCK_SIZE;i++) {
		printf("%.2x ", thiskey[i]);
	}
	printf("\n");
	keyPtr = thiskey;
#warning NOT ON AVR: AES not included!
#endif
}

void aes_init(const uint8_t * key, uint8_t len) {
	aes_setKey(key, len);
}

// Verschl�sselungs-Kram:
typedef union {
	struct {
		uint32_t sensorAddress;
		uint32_t sequenceNumber;
		uint8_t blockNumber;
	};
	uint8_t byteblock[BLOCK_SIZE];
} EncryptionBlock;

void aes_cryptPayload(uint8_t * payload, uint8_t payloadLength, uint32_t sensorAddress, uint32_t sequenceNumber, uint8_t incoming) {
	uint8_t blockNumber = incoming ? 0 : 128;
	uint8_t bufferPosition = 0;
	
//	printf("En/Decrypting len=%d with incoming=%d, sensAddr=0x%x, seqNum=%d.\n", payloadLength, incoming, sensorAddress, sequenceNumber);
	
/*
	uint8_t i;
	printf("Before crypt: ");
	for(i=0;i<payloadLength;i++)
		printf("%.2x ", payload[i]);
	printf("\n");
	i = payloadLength;
*/
	
	do {
		// OTP-Block erzeugen:
		EncryptionBlock block;
		// Zero memory:
		uint8_t len = BLOCK_SIZE;
		while (len--)
			block.byteblock[len] = 0;

		// Ursprungsdaten einf�llen
		block.sensorAddress = sensorAddress;
		block.sequenceNumber = sequenceNumber;
		block.blockNumber = blockNumber;
		
#ifdef _AVR_IO_H_
		// Verschl�sseln:
		aes_encrypt((uint8_t*)&block, &expandedEncryptionKey[0]);
#else
		uint8_t i;
		printf("Encrypting on AVR block: ");
		for (i=0;i<BLOCK_SIZE;i++) {
			printf("%.2x ", block.byteblock[i]);
		}
		
		if (keyPtr) {
			printf("with key ");
			for (i=0;i<BLOCK_SIZE;i++) {
				printf("%.2x ", keyPtr[i]);
			}
			printf("\n AES Result: ");
			AES_ECB_encrypt(&block.byteblock[0], keyPtr, &block.byteblock[0], BLOCK_SIZE);
			
			for (i=0;i<BLOCK_SIZE;i++) {
				printf("%.2x ", block.byteblock[i]);
			}
			printf("\n");
			
		} else {
			printf("- no key known.\n");
		}
#endif
		
		do {
			if (!payloadLength--) {
				return;
			}
			payload[bufferPosition] ^= block.byteblock[bufferPosition & (BLOCK_SIZE-1)];
		} while ((++bufferPosition & (BLOCK_SIZE-1)));
	} while (++blockNumber);
}

