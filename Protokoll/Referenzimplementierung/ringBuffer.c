/*
 * ringBuffer.c
 *
 * Created: 27.10.2017 13:52:46
 *  Author: thagemeier
 */ 

#include "ringBuffer.h"
#include <avr/interrupt.h>

// Größe ca: 344 Bytes

RB_BUFFER_t * rb_createBuffer(void * data, unsigned int buflen) {
	RB_BUFFER_t * buf = (RB_BUFFER_t*)data;
	if (buflen > (sizeof(RB_BUFFER_t) + 2)) {
		buf->buffer = ((unsigned char *)data) + sizeof(RB_BUFFER_t);
		buf->readPosition = 0;
		buf->writePosition = 0;
		buf->size = 0;
		buf->maxSize = buflen - sizeof(RB_BUFFER_t);
		return buf;
	} else {
		return (RB_BUFFER_t*)0;
	}
}

unsigned char rb_peek(RB_BUFFER_t * buf, unsigned char position) {
	if (position < buf->size) {
		position += buf->readPosition;
		if (!(position < buf->maxSize))
			position -= buf->maxSize;
		return buf->buffer[position];
	}
	return 0;
}

void rb_delete(RB_BUFFER_t * buf, unsigned char howMany) {
	if (howMany && (howMany < buf->size)) {
		buf->readPosition += howMany;
		buf->size -= howMany;
		// Wrap if necessary:
		if (!(buf->readPosition < buf->maxSize))
			buf->readPosition -= buf->maxSize;
	} else { // Alles löschen
		buf->readPosition = buf->writePosition;
		buf->size = 0;
	}
}

unsigned char rb_put(RB_BUFFER_t * buf, unsigned char whatChar) {
	if (buf->size < buf->maxSize) {
		buf->buffer[buf->writePosition++] = whatChar;
		buf->size++;
		// Wrap around if necessary:
		if (!(buf->writePosition < buf->maxSize))
			buf->writePosition = 0; // Das geht nur, da wir jedes Mal höchstens ein Byte hinzufügen
		return 1;
	} else {
		return 0;
	}
}

unsigned char rb_get(RB_BUFFER_t * buf) {
	unsigned char data = rb_peek(buf, 0);
	rb_delete(buf, 1);
	return data;
}

unsigned char rb_getCount(RB_BUFFER_t * buf) {
	return buf->size;
}

unsigned char rb_peek_sync(RB_BUFFER_t * buf, unsigned char position) {
	unsigned char data;
	unsigned char stmp = SREG;
	if (position < buf->size) {
		position += buf->readPosition;
		if (!(position < buf->maxSize))
			position -= buf->maxSize;
		data = buf->buffer[position];
		SREG = stmp;
		return data;
	}
	SREG = stmp;
	return 0;
}

void rb_delete_sync(RB_BUFFER_t * buf, unsigned char howMany) {
	unsigned char stmp = SREG;
	cli(); // Unkritisch
	if (howMany && (howMany < buf->size)) {
		buf->readPosition += howMany;
		buf->size -= howMany;
		if (!(buf->readPosition < buf->maxSize))
			buf->readPosition -= buf->maxSize;
	} else { // Alles löschen
		buf->readPosition = 0;
		buf->writePosition = 0;
		buf->size = 0;
	}
	SREG = stmp;
}

unsigned char rb_put_sync(RB_BUFFER_t * buf, unsigned char whatChar) {
	unsigned char stmp = SREG;
	cli(); // Unkritisch
	if (buf->size < buf->maxSize) {
		buf->buffer[buf->writePosition++] = whatChar;
		buf->size++;
		if (!(buf->writePosition < buf->maxSize))
			buf->writePosition = 0; // Das geht nur, da wir jedes Mal höchstens ein Byte hinzufügen
		SREG = stmp;
		return 1;
	} else {
		SREG = stmp;
		return 0;
	}
}

unsigned char rb_get_sync(RB_BUFFER_t * buf) {
	unsigned char data = rb_peek_sync(buf, 0);
	rb_delete_sync(buf, 1);
	return data;
}

unsigned char rb_getCount_sync(RB_BUFFER_t * buf) {
	unsigned char count;
	unsigned char stmp = SREG;
	cli(); // Unkritisch
	count = buf->size;
	SREG = stmp;
	return count;
}
