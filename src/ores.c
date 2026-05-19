#include "ores.h"

// list of all existing ores
ore_t ores[] = {
  {
    .ore_name   = "coal",
    .block_type = BLOCK_TYPE_COAL,
    .min_y = 5,
    .max_y = 50,
    .spawn_chance = 0.6f,
    .vein_min_size = 2,
    .vein_max_size = 4
  },
  {
    .ore_name   = "iron",
    .block_type = BLOCK_TYPE_IRON,
    .min_y = 5,
    .max_y = 50,
    .spawn_chance = 0.3f,
    .vein_min_size = 2,
    .vein_max_size = 4
  },
  {
    .ore_name   = "gold",
    .block_type = BLOCK_TYPE_GOLD,
    .min_y = 5,
    .max_y = 50,
    .spawn_chance = 0.2f,
    .vein_min_size = 2,
    .vein_max_size = 2
  },
  {
    .ore_name   = "diamond",
    .block_type = BLOCK_TYPE_DIAMOND,
    .min_y = 5,
    .max_y = 50,
    .spawn_chance = 0.1f,
    .vein_min_size = 2,
    .vein_max_size = 2
  },
};

void place_ore(int32_t z_row, int32_t x_col, ore_type_t ore_type){
  ore_t ore = ores[ore_type];
  uint32_t seed = (uint32_t)(z_row * 73856093u) ^ (uint32_t)(x_col * 19349663u); // two prime numbers for hashing
  bool will_spawn = random_spawn_from_seed(seed, ore.spawn_chance);

  if(!will_spawn) return;

  uint32_t ore_y_seed = seed ^ 0xA53A9B1Du; // generate two more seeds, one for y, and one for ore size
  uint32_t ore_size_seed = seed ^ 0xF12C77B3u;

  int32_t ore_y    = (double)ore.min_y + (double)(ore.max_y - ore.min_y) * (double)(rand_i(ore_y_seed) / (double)UINT32_MAX);
  int32_t ore_size = (double)ore.vein_min_size + (double)(ore.vein_max_size - ore.vein_min_size + 1) * (double)(rand_i(ore_size_seed) / (double)UINT32_MAX);

  for(int32_t y = ore_y; y < ore_y + ore_size; y++){
    for(int32_t z = z_row; z < z_row + ore_size; z++){
      for(int32_t x = x_col; x < x_col + ore_size; x++){
        assign_block_material(y, z, x, ores[ore_type].block_type);
        block_at(y, z, x).solid = true;
      }
    }
  }
}

void place_ores(){
  // place ores
  for(uint32_t z = 32; z < MAP_NUM_VOXELS_Z - 32; z++){
    for(uint32_t x = 32; x < MAP_NUM_VOXELS_X - 32; x++){
      place_ore(z, x, ORE_TYPE_COAL);
      place_ore(z, x, ORE_TYPE_IRON);
      place_ore(z, x, ORE_TYPE_GOLD);
      place_ore(z, x, ORE_TYPE_DIAMOND);
    }
  }
}