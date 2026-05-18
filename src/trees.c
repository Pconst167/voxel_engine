#include "trees.h"
#include "array.h"

tree_t *trees;

void init_trees(){
  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree0",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree1",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK1,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree2",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK2,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree3",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK3,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree4",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK4,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree5",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK5,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree6",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK6,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );

  array_push(
    trees, 
    (tree_t){
      .tree_name = "tree7",
      .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
      .leaves_block_type = BLOCK_TYPE_LEAVES,
      .min_height = 4,
      .max_height = 10,
      .min_width = 1,
      .max_width = 2,
      .rarity = 1.0f
    }
  );
}

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
  
  int32_t tree_bottom = first_solid_y - 3; // -3 to place trunk below ground
  int32_t tree_top    = first_solid_y + tree.max_height;
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