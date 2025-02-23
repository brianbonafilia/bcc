#ifndef SRC_ARENA_H
#define SRC_ARENA_H

#include <stdlib.h>

typedef struct {
  void* to_free_ptr;
  const int size;
  int used;
  void* next_ptr;
} Arena;

Arena allocate_arena(int size_in_bytes);
void* arena_alloc(Arena* arena, int size_in_bytes);
void release(Arena* arena);



#endif
