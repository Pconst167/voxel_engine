#ifndef TREES_H
#define TREES_H

#include "block.h"

typedef enum{
  TREE0,
  TREE1,
  TREE2,
  TREE3,
  TREE4,
  TREE5,
  TREE6,
  TREE7,
  TREE8,
  TREE9
} tree_type_t;

typedef struct{
  char *tree_name;
  block_type_t trunk_block_type;
  block_type_t leaves_block_type;
  uint32_t min_height;
  uint32_t max_height;
  uint32_t min_width;
  uint32_t max_width;
  float spawn_chance;
} tree_t;

extern tree_t trees[];

void place_tree(int32_t z_row, int32_t x_col, tree_type_t tree_type);
void place_trees();

#endif