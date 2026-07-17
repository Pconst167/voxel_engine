#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <stdbool.h>
#include "block.h"

typedef struct{
  block_t blocks[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
} save_state_t;

bool save_chunk(int32_t chunk_y, int32_t chunk_z, int32_t chunk_x);
bool load_all_saved_chunks(void);
bool load_chunk(int32_t chunk_y, int32_t chunk_z, int32_t chunk_x);

#endif