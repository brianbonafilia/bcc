#include "arena.h"
#include <stdlib.h>
#include <stdio.h>

Arena allocate_arena(int size_in_bytes) {
  Arena arena = {
      .next_ptr= malloc(size_in_bytes),
      .used = 0,
      .size = size_in_bytes
  };
  if (arena.next_ptr == NULL) {
    fprintf(stderr, "failed to allocate memory");
    exit(2);
  }
  arena.to_free_ptr = arena.next_ptr;
  return arena;
}

void* arena_alloc(Arena* arena, int size_in_bytes) {
  if (arena->used + size_in_bytes > arena->size) {
    fprintf(stderr, "out of memory in arena");
    exit(2);
  }
  void* ret = arena->next_ptr;
  arena->next_ptr += size_in_bytes;
  return ret;
}

void release(Arena* arena) {
  free(arena->to_free_ptr);
}

