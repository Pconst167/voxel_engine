#ifndef TREES_H
#define TREES_H

#include "block.h"
#include "biome.h"

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
  TREE9,
  FLOWER_TREE0,
  CHERRY_TREE0,

  CACTUS0
} tree_type_t;

typedef enum{
  PLANT_SPECIES_TREE,
  PLANT_SPECIES_CACTUS
} plant_species_t;

typedef struct{
  plant_species_t plant_species;
  char *tree_name;
  block_type_t trunk_block_type;
  block_type_t leaves_block_type;
  int32_t min_height;
  int32_t max_height;
  int32_t min_width;
  int32_t max_width;
  float spawn_chance;
  uint32_t biome_mask;
} tree_t;

extern tree_t trees[];

void place_tree(int32_t z_row, int32_t x_col, tree_type_t tree_type);
void place_trees();

#endif