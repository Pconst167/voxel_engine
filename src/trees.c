#include "trees.h"
#include "log.h"

tree_t trees[] = {
  {
    .tree_name = "tree0",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_LEAVES0,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree1",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK1,
    .leaves_block_type = BLOCK_TYPE_LEAVES1,
    .min_height = 10,
    .max_height = 25,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree2",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK2,
    .leaves_block_type = BLOCK_TYPE_LEAVES2,
    .min_height = 10,
    .max_height = 35,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree3",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK3,
    .leaves_block_type = BLOCK_TYPE_LEAVES3,
    .min_height = 8,
    .max_height = 28,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree4",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK4,
    .leaves_block_type = BLOCK_TYPE_LEAVES4,
    .min_height = 6,
    .max_height = 20,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree5",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK5,
    .leaves_block_type = BLOCK_TYPE_LEAVES5,
    .min_height = 10,
    .max_height = 25,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree6",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK6,
    .leaves_block_type = BLOCK_TYPE_LEAVES6,
    .min_height = 8,
    .max_height = 32,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree7",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES7,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree8",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES8,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  },
  {
    .tree_name = "tree9",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES9,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .rarity = 1.0f
  }
};

void place_tree(int32_t z_row, int32_t x_col, tree_type_t tree_type){
  tree_t tree = trees[tree_type];

  int32_t first_solid_y = 0;
  // find first solid block along y
  for(int32_t y = MAP_NUM_VOXELS_Y - 1; y >= 0; y--){
    if(block_at(y, z_row, x_col).solid){
      first_solid_y = y;
      break;
    }
  }

  uint32_t height = (double)tree.min_height + (double)(tree.max_height - tree.min_height) * (double)(rand_i(z_row * x_col) / (double)UINT32_MAX);

  int32_t tree_bottom = first_solid_y - 3; // -3 to place trunk below ground
  int32_t tree_top    = first_solid_y + height;
  int32_t tree_middle = (tree_bottom + tree_top) / 2;

  // leaves
  for(int32_t y = tree_middle; y <= tree_top; y++){
    for(int32_t z = z_row - 3; z <= z_row + 3; z++){
      for(int32_t x = x_col - 3; x <= x_col + 3; x++){
        if(xorshift32() % 2 == 0){
          assign_block_material(y, z, x, tree.leaves_block_type);
          block_at(y, z, x).solid = true;
        }
        else{
          assign_block_material(y, z, x, BLOCK_TYPE_AIR);
          block_at(y, z, x).solid = false;
        }
      }
    }
  }

  // trunk
  for(int32_t y  = tree_bottom; y <= tree_top; y++){
    assign_block_material(y, z_row, x_col, tree.trunk_block_type);
    block_at(y, z_row, x_col).solid = true;
  }
}