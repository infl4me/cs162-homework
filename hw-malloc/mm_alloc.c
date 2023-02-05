/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

struct block {
  struct block* prev;
  struct block* next;
  char free;
  size_t size;
};

static struct block* list = NULL;

void* mm_malloc(size_t size) {
  if (size == 0) {
    return NULL;
  }

  struct block* p = list;

  // if no blocks yet
  if (p == NULL) {
    list = p = sbrk(sizeof(struct block) + size);
    if ((char *) list == (char *) -1)
      return NULL;

    p->size = size;
    p->free = 0;
    p->prev = NULL;
    p->next = NULL;
    return p + sizeof(struct block);
  }

  // find a free block that would fit or the last one
  while (p->next) {
    if (p->free && p->size >= size) {
      break;
    }
    p = p->next;
  }

  // if free no block, allocate a new one
  if (!(p->free && p->size >= size)) {
    struct block* next_p = sbrk(sizeof(struct block) + size);
    if ((char *) next_p == (char *) -1)
      return NULL;

    p->next = next_p;

    next_p->prev = p;
    next_p->next = NULL;
    next_p->free = 0;
    next_p->size = size;

    return next_p + sizeof(struct block);
  }

  // if the free block has bigger size and can contain another block, than split it in two
  // otherwise just return it as it is
  if (p->size - (size + sizeof(struct block)) > sizeof(struct block)) {
    struct block* next_p = p + sizeof(struct block) + size;
    next_p->free = 1;
    next_p->prev = p;
    next_p->next = p->next;
    next_p->size = p->size - (size + sizeof(struct block));

    p->free = 0;
    p->size = size;
    p->next = next_p;
    return p + sizeof(struct block);
  } else {
    p->free = 0;
    return p + sizeof(struct block);
  }
}

void* mm_realloc(void* ptr, size_t size) {
  if (list == NULL)
    return NULL;
  if (ptr == NULL)
    return mm_malloc(size);
  if (size == 0) {
    mm_free(ptr);
    return NULL;
  }

  struct block* p = ptr - sizeof(struct block);

  if (p->size >= size) return ptr;

  struct block* next_p = sbrk(sizeof(struct block) + size);
  if ((char *) next_p == (char *) -1)
    return NULL;

  p->free = 1;

  while (p->next) {
    p = p->next;
  }

  next_p->prev = p;
  next_p->next = NULL;
  next_p->free = 0;
  next_p->size = size;

  return next_p + sizeof(struct block);
}

void mm_free(void* ptr) {
  if (list == NULL || ptr == NULL)
    return;

  struct block* p = ptr - sizeof(struct block);
  p->free = 1;
}
