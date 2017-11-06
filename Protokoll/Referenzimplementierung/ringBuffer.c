/*
 * ringBuffer.c
 *
 * Created: 27.10.2017 13:52:46
 *  Author: thagemeier
 */ 

#include "ringBuffer.h"
#ifndef NO_AVR
#include <avr/interrupt.h>
#endif

// Größe ca: 344 Bytes

RB_BUFFER_t * rb_createBuffer(void * data, uint16_t buflen) {
	RB_BUFFER_t * buf = (RB_BUFFER_t*)data;
	if (buflen > (sizeof(RB_BUFFER_t) + 2)) {
		buf->buffer = ((uint8_t *)data) + sizeof(RB_BUFFER_t);
		buf->readPosition = 0;
		buf->writePosition = 0;
		buf->size = 0;
		buf->maxSize = buflen - sizeof(RB_BUFFER_t);
		return buf;
	} else {
		return (RB_BUFFER_t*)0;
	}
}

uint8_t rb_peek(RB_BUFFER_t * buf, uint8_t position) {
	if (position < buf->size) {
		position += buf->readPosition;
		if (!(position < buf->maxSize))
			position -= buf->maxSize;
		return buf->buffer[position];
	}
	return 0;
}

void rb_delete(RB_BUFFER_t * buf, uint8_t howMany) {
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

uint8_t rb_put(RB_BUFFER_t * buf, uint8_t whatChar) {
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

uint8_t rb_get(RB_BUFFER_t * buf) {
	uint8_t data = rb_peek(buf, 0);
	rb_delete(buf, 1);
	return data;
}

uint8_t rb_getCount(RB_BUFFER_t * buf) {
	return buf->size;
}

uint8_t rb_peek_sync(RB_BUFFER_t * buf, uint8_t position) {
	uint8_t data;
#ifndef NO_AVR
	uint8_t stmp = SREG;
	cli();
#endif
	if (position < buf->size) {
		position += buf->readPosition;
		if (!(position < buf->maxSize))
			position -= buf->maxSize;
		data = buf->buffer[position];
#ifndef NO_AVR
		SREG = stmp;
#endif
		return data;
	}
#ifndef NO_AVR
	SREG = stmp;
#endif
	return 0;
}

void rb_delete_sync(RB_BUFFER_t * buf, uint8_t howMany) {
#ifndef NO_AVR
	uint8_t stmp = SREG;
	cli(); // Unkritisch
#endif
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
#ifndef NO_AVR
	SREG = stmp;
#endif
}

uint8_t rb_put_sync(RB_BUFFER_t * buf, uint8_t whatChar) {
#ifndef NO_AVR
	uint8_t stmp = SREG;
	cli(); // Unkritisch
#endif
	if (buf->size < buf->maxSize) {
		buf->buffer[buf->writePosition++] = whatChar;
		buf->size++;
		if (!(buf->writePosition < buf->maxSize))
			buf->writePosition = 0; // Das geht nur, da wir jedes Mal höchstens ein Byte hinzufügen
#ifndef NO_AVR
		SREG = stmp;
#endif
		return 1;
	} else {
#ifndef NO_AVR
		SREG = stmp;
#endif
		return 0;
	}
}

uint8_t rb_get_sync(RB_BUFFER_t * buf) {
	uint8_t data = rb_peek_sync(buf, 0);
	rb_delete_sync(buf, 1);
	return data;
}

uint8_t rb_getCount_sync(RB_BUFFER_t * buf) {
	uint8_t count;
#ifndef NO_AVR
	uint8_t stmp = SREG;
	cli(); // Unkritisch
#endif
	count = buf->size;
#ifndef NO_AVR
	SREG = stmp;
#endif
	return count;
}
