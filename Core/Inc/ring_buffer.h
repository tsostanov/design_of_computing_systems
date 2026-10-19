#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/* Storage and synchronization belong to the caller. All capacity bytes are usable. */
typedef struct {
    uint8_t *data;
    size_t capacity, head, tail, count;
} RingBuffer;
void Ring_Init(RingBuffer *ring, uint8_t *storage, size_t capacity);
size_t Ring_Free(const RingBuffer *ring);
bool Ring_Write(RingBuffer *ring, const uint8_t *data, size_t size);
bool Ring_Peek(const RingBuffer *ring, uint8_t *value);
bool Ring_Read(RingBuffer *ring, uint8_t *value);
#endif
