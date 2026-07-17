#ifndef DEBUG_H
#define DEBUG_H

#include <stdint.h>
#include <stddef.h>

#define DEBUG_MEMORY_ALLOCATION_SIZE 2000

void *debug_memory_malloc(size_t size, const char *file, uint32_t line);
void debug_print_allocated(void);

#ifdef DEBUG_MEMORY

  #define malloc(size) debug_memory_malloc((size), __FILE__, __LINE__)

#endif

#endif