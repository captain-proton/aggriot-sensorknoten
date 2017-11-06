/*
 * ringBuffer.h
 *
 * Created: 27.10.2017 13:53:10
 *  Author: thagemeier
 */ 

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

typedef struct {
	uint16_t readPosition;
	uint16_t writePosition;
	uint16_t size;
	uint16_t maxSize;
	uint8_t * buffer;
} RB_BUFFER_t;

RB_BUFFER_t * rb_createBuffer(void * data, uint16_t buflen);
uint8_t rb_peek(RB_BUFFER_t * buf, uint8_t position);
void rb_delete(RB_BUFFER_t * buf, uint8_t howMany);
uint8_t rb_put(RB_BUFFER_t * buf, uint8_t whatChar);
uint8_t rb_get(RB_BUFFER_t * buf);
uint8_t rb_getCount(RB_BUFFER_t * buf);

uint8_t rb_peek_sync(RB_BUFFER_t * buf, uint8_t position);
void rb_delete_sync(RB_BUFFER_t * buf, uint8_t howMany);
uint8_t rb_put_sync(RB_BUFFER_t * buf, uint8_t whatChar);
uint8_t rb_get_sync(RB_BUFFER_t * buf);
uint8_t rb_getCount_sync(RB_BUFFER_t * buf);

#endif
