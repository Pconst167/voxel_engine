#ifndef ORE_H
#define ORE_H

#include <stdint.h>
#include "block.h"

typedef enum{
  ORE_TYPE_COAL,
  ORE_TYPE_IRON,
  ORE_TYPE_GOLD,
  ORE_TYPE_DIAMOND
} ore_type_t;

typedef struct{
  char *ore_name;
  block_type_t block_type;
  float spawn_chance;
  int32_t min_y;
  int32_t max_y;
  int32_t vein_min_size;
  int32_t vein_max_size;
} ore_t;

extern ore_t ores[];

void place_ore(int32_t z_row, int32_t x_col, ore_type_t ore_type);
void place_ores();

#endif