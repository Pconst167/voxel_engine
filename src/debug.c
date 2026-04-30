#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "log.h"

/* Important: include debug.h after stdlib.h, then disable malloc macro here. */
#include "debug.h"

#ifdef malloc
  #undef malloc
#endif

typedef struct{
  char file[256];
  uint32_t line;
  size_t size;
} memory_allocation_t;

static memory_allocation_t memory_allocations[DEBUG_MEMORY_ALLOCATION_SIZE];
static uint32_t num_allocations = 0;

void *debug_memory_malloc(size_t size, const char *file, uint32_t line){
  void *memory_pointer = malloc(size);

  if(memory_pointer != NULL && num_allocations < DEBUG_MEMORY_ALLOCATION_SIZE){
    memory_allocations[num_allocations].size = size;
    memory_allocations[num_allocations].line = line;

    strcpy(memory_allocations[num_allocations].file, file);

    num_allocations++;
  }

  return memory_pointer;
}

void debug_print_allocated(void){
  uint32_t total_memory = 0;

  for(uint32_t allocation_index = 0; allocation_index < num_allocations; allocation_index++){
    printf("file: %s, line: %u, size: %zu bytes, %.3fMB\n", 
      memory_allocations[allocation_index].file,
      memory_allocations[allocation_index].line,
      memory_allocations[allocation_index].size,
      (float)(memory_allocations[allocation_index].size) / (1024.0f * 1024.0f)
    );
    
    total_memory += memory_allocations[allocation_index].size;
  }

  print_log("total allocations: %u\n", num_allocations);
  print_log("total memory: %u bytes, %.3fMB\n", total_memory, (float)(total_memory) / (1024.0f * 1024.0f));
}