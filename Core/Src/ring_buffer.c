#include "ring_buffer.h"
void Ring_Init(RingBuffer *r, uint8_t *storage, size_t capacity)
{
    r->data = storage;
    r->capacity = capacity;
    r->head = r->tail = r->count = 0;
}
size_t Ring_Free(const RingBuffer *r) { return r->capacity - r->count; }
bool Ring_Write(RingBuffer *r, const uint8_t *data, size_t size)
{
    if (size > Ring_Free(r)) return false;
    for (size_t i = 0; i < size; ++i) {
        r->data[r->head] = data[i];
        r->head = (r->head + 1U) % r->capacity;
    }
    r->count += size;
    return true;
}
bool Ring_Peek(const RingBuffer *r, uint8_t *value)
{
    if (r->count == 0) return false;
    *value = r->data[r->tail];
    return true;
}
bool Ring_Read(RingBuffer *r, uint8_t *value)
{
    if (!Ring_Peek(r, value)) return false;
    r->tail = (r->tail + 1U) % r->capacity;
    --r->count;
    return true;
}
