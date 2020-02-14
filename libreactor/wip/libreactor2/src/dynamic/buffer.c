#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "buffer.h"

static size_t roundup(size_t size)
{
  size --;
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size |= size >> 32;
  size ++;

  return size;
}

/* constructor */

void buffer_construct(buffer *b)
{
  *b = (buffer) {0};
}

void buffer_destruct(buffer *b)
{
  buffer_clear(b);
}

/* capacity */

size_t buffer_size(buffer *b)
{
  return b->size;
}

size_t buffer_capacity(buffer *b)
{
  return b->capacity;
}

void buffer_reserve(buffer *b, size_t capacity)
{
  void *data;

  if (capacity > b->capacity)
    {
      capacity = roundup(capacity);
      data = realloc(b->data, capacity);
      b->data = data;
      b->capacity = capacity;
    }
}

void buffer_compact(buffer *b)
{
  void *data;

  if (b->capacity > b->size)
    {
      data = realloc(b->data, b->size);
      b->data = data;
      b->capacity = b->size;
    }
}

/* modifiers */

void buffer_insert(buffer *b, size_t position, void *data, size_t size)
{
  buffer_reserve(b, b->size + size);

  if (position < b->size)
    memmove((char *) b->data + position + size, (char *) b->data + position, b->size - position);
  memcpy((char *) b->data + position, data, size);
  b->size += size;
}

void buffer_erase(buffer *b, size_t position, size_t size)
{
  memmove((char *) b->data + position, (char *) b->data + position + size, b->size - position - size);
  b->size -= size;
}

void buffer_clear(buffer *b)
{
  free(b->data);
  buffer_construct(b);
}

/* element access */

void *buffer_data(buffer *b)
{
  return b->data;
}
