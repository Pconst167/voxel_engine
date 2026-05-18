#ifndef TREES_H
#define TREES_H

#include "block.h"

typedef enum{
  TREE0 = 0,
  TREE1 = 1,
  TREE2 = 2,
  TREE3 = 3,
  TREE4 = 4,
  TREE5 = 5,
  TREE6 = 6,
  TREE7 = 7
} tree_type_t;

typedef struct{
  char *tree_name;
  block_type_t trunk_block_type;
  block_type_t leaves_block_type;
  uint32_t min_height;
  uint32_t max_height;
  uint32_t min_width;
  uint32_t max_width;
  float rarity;
} tree_t;

extern tree_t *trees;

void place_tree(int32_t z_row, int32_t x_col, tree_type_t tree_type);
void init_trees();

#endif