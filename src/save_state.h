#ifndef SAVE_STATE_H
#define SAVE_STATE_H

#include <stdbool.h>
#include "block.h"

typedef struct{
  vector3_t player_position;
  vector3_t player_direction;

  block_t blocks[MAP_NUM_BLOCKS_Y][MAP_NUM_BLOCKS_Z][MAP_NUM_BLOCKS_X];
} save_state_t;

bool save();
bool load();

#endif