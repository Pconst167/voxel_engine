#include "trees.h"
#include "block.h"
#include "log.h"

tree_t trees[] = {
  {
    .tree_name = "tree0",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_LEAVES0,
    .min_height = 10,
    .max_height = 35,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f
  },
  {
    .tree_name = "tree1",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK1,
    .leaves_block_type = BLOCK_TYPE_LEAVES1,
    .min_height = 10,
    .max_height = 25,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.4f
  },
  {
    .tree_name = "tree2",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK2,
    .leaves_block_type = BLOCK_TYPE_LEAVES2,
    .min_height = 10,
    .max_height = 35,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.3f
  },
  {
    .tree_name = "tree3",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK3,
    .leaves_block_type = BLOCK_TYPE_LEAVES3,
    .min_height = 8,
    .max_height = 28,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.35f
  },
  {
    .tree_name = "tree4",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK4,
    .leaves_block_type = BLOCK_TYPE_LEAVES4,
    .min_height = 6,
    .max_height = 20,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.05f
  },
  {
    .tree_name = "tree5",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK5,
    .leaves_block_type = BLOCK_TYPE_LEAVES5,
    .min_height = 10,
    .max_height = 25,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.08f
  },
  {
    .tree_name = "tree6",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK6,
    .leaves_block_type = BLOCK_TYPE_LEAVES6,
    .min_height = 8,
    .max_height = 32,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.25f
  },
  {
    .tree_name = "tree7",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES7,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.3f
  },
  {
    .tree_name = "tree8",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES8,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.09f
  },
  {
    .tree_name = "tree9",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES9,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.04f
  },

  // FLOWER TREES
  {
    .tree_name = "flowertree0",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_FLOWER_LEAVES0,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.2f
  },
  {
    .tree_name = "flowertree1",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_FLOWER_LEAVES0,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.9f
  }
};

void place_tree(int32_t z_row, int32_t x_col, tree_type_t tree_type){
  tree_t tree = trees[tree_type];

  uint32_t seed = (uint32_t)(z_row * 73856093u) ^ (uint32_t)(x_col * 19349663u); // two prime numbers for hashing
  bool will_spawn = random_spawn_from_seed(seed, tree.spawn_chance);

  if(!will_spawn) return;

  // find first solid block along y
  int32_t first_solid_y = 0;
  for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
    if(block_materials[block_at(y, z_row, x_col).type].solid){
      first_solid_y = y;
      break;
    }
  }

  if(first_solid_y < 0 || first_solid_y >= MAP_NUM_BLOCKS_Y) return;

  uint32_t tree_width_seed  = seed ^ 0xA53A9B1Du; // generate two more seeds, one for y, and one for ore size
  uint32_t tree_height_seed = seed ^ 0xF12C77B3u;

  int32_t height      = (double)tree.min_height + (double)(tree.max_height - tree.min_height) * (double)(rand_i(tree_height_seed) / (double)UINT32_MAX);
  int32_t trunk_width = (double)tree.min_width  + (double)(tree.max_width - tree.min_width + 1) * (double)(rand_i(tree_width_seed) / (double)UINT32_MAX);
  
  int32_t tree_bottom = first_solid_y - 3; // -3 to place trunk below ground
  int32_t tree_top    = first_solid_y + height - 1;
  int32_t tree_middle = (tree_bottom + tree_top) / 2;

  // leaves
  for(int32_t y = tree_middle; y <= tree_top; y++){
    for(int32_t z = z_row - 6; z <= z_row + 6; z++){
      for(int32_t x = x_col - 6; x <= x_col + 6; x++){
        if(xorshift32() % 4 == 0){
          assign_block_material(y, z, x, tree.leaves_block_type);
        }
      }
    }
  }
  
  // trunk
  for(int32_t y = tree_bottom; y <= tree_top - 2; y++){
    float t = (float)(y - tree_bottom) / (float)(tree_top - tree_bottom);
    int32_t width = lerp(t, trunk_width, 1.0f);
    for(int32_t z = z_row - width / 2.0f; z < z_row + width / 2.0f; z++){
      for(int32_t x = x_col - width / 2.0f; x < x_col + width / 2.0f; x++){
        assign_block_material(y, z, x, tree.trunk_block_type);
      }
    }
  }
}

void place_trees(){
  int32_t step_size_x = 9;
  int32_t step_size_z = 9;

  // place trees
  for(uint32_t z = 32; z < MAP_NUM_BLOCKS_Z - 32; z += step_size_z){
    for(uint32_t x = 32; x < MAP_NUM_BLOCKS_X - 32; x += step_size_x){
      int32_t rand = xorshift32();
      if(rand % 5 == 0)
        place_tree(z, x, TREE0);
      else if(rand % 5 == 1)
        place_tree(z, x, TREE1);
      else if(rand % 5 == 2)
        place_tree(z, x, TREE2);
      else if(rand % 5 == 3)
        place_tree(z, x, TREE3);
      else if(rand % 5 == 4)
        place_tree(z, x, FLOWER_TREE0);
    }
  }
}