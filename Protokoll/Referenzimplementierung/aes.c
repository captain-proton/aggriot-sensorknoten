/*
 * aes.c
 *
 * Created: 27.10.2017 18:01:39
 *  Author: thagemeier
 */ 
#include "aes.h"

// These functions are provided by the assembler file. They should not be accessed from anywhere else but here:

void aes_encrypt(unsigned char* text16, unsigned char* expandedkey176);
#ifdef NEED_DECRYPTION_FEATURE
void aes_decrypt(unsigned char* text16, unsigned char* expandedkey176);
#endif
void aes_expand_key(unsigned char* key, unsigned char* expandedkeybuf176);
void aes_patch_key(unsigned char* key);

unsigned char expandedEncryptionKey[EXPANDED_KEY_SIZE];
#ifdef NEED_DECRYPTION_FEATURE
unsigned char expandedDecryptionKey[EXPANDED_KEY_SIZE];
#endif
unsigned char AESkeyBuffer[KEY_SIZE] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};
unsigned char AEStextBuffer[BLOCK_SIZE];
unsigned char aesStatus = (1 << AES_NEED_KEY_UPDATE);
unsigned char nextTextByte = 0;

void AESCryptGenerateKey(const unsigned char* thiskey, unsigned char len) {
	// The key that is passed to this function is buffered in the AESkeyBuffer
	// so if the same key is passed again the expanded keys don't have to be
	// regenerated.
	unsigned char keyBufIndex = 0;
	
	if (len != KEY_SIZE) {
		aesStatus |= (1 << AES_NEED_KEY_UPDATE);
	}
	
	// Copy provided key to internal buffer
	while (len--) {
		if (AESkeyBuffer[keyBufIndex] != *thiskey) {
			AESkeyBuffer[keyBufIndex] = *thiskey;
			aesStatus |= (1 << AES_NEED_KEY_UPDATE);
		}
		keyBufIndex++;
		thiskey++;
		if (keyBufIndex == KEY_SIZE) {
			break;
		}
	}
	
	// If the key needs an update (has been changed or generated for the first time)
	// we need to update the extended encryption/decryption key which is done here:
	if (aesStatus & (1 << AES_NEED_KEY_UPDATE)) {
		// Expand the key to the encryption key:
		aes_expand_key(&AESkeyBuffer[0], &expandedEncryptionKey[0]);
		
#ifdef NEED_DECRYPTION_FEATURE
		// Copy the result to the decryption key:
		keyBufIndex = EXPANDED_KEY_SIZE;
		while (keyBufIndex--) {
			expandedDecryptionKey[keyBufIndex] = expandedEncryptionKey[keyBufIndex];
		}
		// Run patch on decryption key:
		aes_patch_key(&expandedDecryptionKey[0]);
#endif
		// We have just updated the key - no update needed:
		aesStatus &= ~(1 << AES_NEED_KEY_UPDATE);
	}
}

// Initialize the buffer to basic state:
void aes_init(void) {
	// Clear the message buffer:
	unsigned char i = BLOCK_SIZE;
	while (i--) {
		AEStextBuffer[i] = 0;
	}
	nextTextByte = 0; // Wraps around at next increment
	aesStatus &= ~(1 << AES_NEED_ENCRYPTION); // Stop that..
}

void AESCryptFeedData(const unsigned char* data, unsigned char len, unsigned char finaldata) {
	// Data XORed with the buffer. When the buffer is full, it is encrypted.
	// The first 16 bytes will be put into the buffer 1:1 but from the 17th byte
	// on the data is first XORed with the result of the previous encryption run
	while (len--) {
		AESCryptFeedOneByte(*data++);
	}
	if (finaldata) {
		AESCryptCalculateNow();
	}
}

void AESCryptFeedOneByte(const unsigned char data) {
	if (!nextTextByte) {
		// This function will only actually do anything if the buffer has been changed after the last crypting
		AESCryptCalculateNow();
	}
	AEStextBuffer[nextTextByte] ^= data;
	nextTextByte = (nextTextByte + 1) % BLOCK_SIZE;
	// Data of the buffer has been changed so we need to encrypt it next time we add a byte/call AESCryptCalculateNow
	aesStatus |= (1 << AES_NEED_ENCRYPTION);
}

void AESCryptCalculateNow(void) {
	// If the AES_NEED_ENCRYPTION-Flag is set: Encrypt the buffer. Otherwise the buffer has been cleared/encrypted
	// already and not changed in between.
	if (aesStatus & (1 << AES_NEED_ENCRYPTION)) {
		// Encrypt the buffer:
		aes_encrypt(&AEStextBuffer[0], &expandedEncryptionKey[0]);
		// Clear NEED_ENCRYPTION-Flag since thats what we have just done.
		aesStatus &= ~(1 << AES_NEED_ENCRYPTION);
	}
}


#endif
