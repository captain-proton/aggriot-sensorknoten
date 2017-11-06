/*
 * aes.c
 *
 * Created: 27.10.2017 18:01:45
 *  Author: thagemeier
 */ 
#include "signatures.h"

#ifndef _AES_H_
#define _AES_H_




void AESCryptGenerateKey(const unsigned char* thiskey, unsigned char len);
void AESCryptInit(void);
void AESCryptFeedData(const unsigned char* data, unsigned char len, unsigned char finaldata);
void AESCryptFeedOneByte(const unsigned char data);
void AESCryptCalculateNow(void);

#define EXPANDED_KEY_SIZE		176
#define KEY_SIZE				16
#define BLOCK_SIZE				16

extern unsigned char AEStextBuffer[BLOCK_SIZE];

#endif
