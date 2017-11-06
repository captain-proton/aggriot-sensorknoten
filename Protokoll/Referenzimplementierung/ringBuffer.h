/*
 * ringBuffer.h
 *
 * Created: 27.10.2017 13:53:10
 *  Author: thagemeier
 */ 

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

typedef struct {
	unsigned int readPosition;
	unsigned int writePosition;
	unsigned int size;
	unsigned int maxSize;
	unsigned char * buffer;
} RB_BUFFER_t;

RB_BUFFER_t * rb_createBuffer(void * data, unsigned int buflen);
unsigned char rb_peek(RB_BUFFER_t * buf, unsigned char position);
void rb_delete(RB_BUFFER_t * buf, unsigned char howMany);
unsigned char rb_put(RB_BUFFER_t * buf, unsigned char whatChar);
unsigned char rb_get(RB_BUFFER_t * buf);
unsigned char rb_getCount(RB_BUFFER_t * buf);

unsigned char rb_peek_sync(RB_BUFFER_t * buf, unsigned char position);
void rb_delete_sync(RB_BUFFER_t * buf, unsigned char howMany);
unsigned char rb_put_sync(RB_BUFFER_t * buf, unsigned char whatChar);
unsigned char rb_get_sync(RB_BUFFER_t * buf);
unsigned char rb_getCount_sync(RB_BUFFER_t * buf);

#endif
