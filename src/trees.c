#include "trees.h"
#include "block.h"
#include "noise.h"
#include "log.h"

typedef struct{
  tree_type_t tree_type;
  float spawn_chance;
} trees_sort_key_t;

tree_t trees[] = {
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree0",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_LEAVES0,
    .min_height = 10,
    .max_height = 35,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f,
    .biome_mask = 1u << BIOME_TYPE_VOLCANIC
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree1",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK1,
    .leaves_block_type = BLOCK_TYPE_LEAVES1,
    .min_height = 10,
    .max_height = 25,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.6f,
    .biome_mask = 1u << BIOME_TYPE_FOREST
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree2",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK2,
    .leaves_block_type = BLOCK_TYPE_LEAVES2,
    .min_height = 10,
    .max_height = 35,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f,
    .biome_mask = 1u << BIOME_TYPE_FOREST | 1u << BIOME_TYPE_RAIN_FOREST
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree3",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK3,
    .leaves_block_type = BLOCK_TYPE_LEAVES3,
    .min_height = 8,
    .max_height = 28,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f,
    .biome_mask = 1u << BIOME_TYPE_SAVANNAH
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree4",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK4,
    .leaves_block_type = BLOCK_TYPE_LEAVES4,
    .min_height = 6,
    .max_height = 20,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f,
    .biome_mask = 1u << BIOME_TYPE_FOREST | 1u << BIOME_TYPE_RAIN_FOREST
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree5",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK5,
    .leaves_block_type = BLOCK_TYPE_LEAVES5,
    .min_height = 10,
    .max_height = 25,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f,
    .biome_mask = 1u << BIOME_TYPE_GRASSLANDS
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree6",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK6,
    .leaves_block_type = BLOCK_TYPE_LEAVES6,
    .min_height = 8,
    .max_height = 32,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.5f,
    .biome_mask = 1u << BIOME_TYPE_RAIN_FOREST
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree7",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES7,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.2f,
    .biome_mask = 1u << BIOME_TYPE_VOLCANIC
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree8",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES8,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.1f,
    .biome_mask = 1u << BIOME_TYPE_RAIN_FOREST
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "tree9",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK7,
    .leaves_block_type = BLOCK_TYPE_LEAVES9,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.3f,
    .biome_mask = 1u << BIOME_TYPE_VOLCANIC
  },

  // FLOWER TREES
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "flowertree0",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_FLOWER_LEAVES0,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.8f,
    .biome_mask = 1u << BIOME_TYPE_GRASSLANDS
  },
  {
    .plant_species = PLANT_SPECIES_TREE,
    .tree_name = "flowertree1",
    .trunk_block_type = BLOCK_TYPE_TREE_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_FLOWER_LEAVES1,
    .min_height = 10,
    .max_height = 30,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.8f,
    .biome_mask = 1u << BIOME_TYPE_GRASSLANDS
  },

  // CACTI
  {
    .plant_species = PLANT_SPECIES_CACTUS,
    .tree_name = "cactus0",
    .trunk_block_type = BLOCK_TYPE_CACTUS_TRUNK0,
    .leaves_block_type = BLOCK_TYPE_CACTUS_FLOWER0,
    .min_height = 2,
    .max_height = 15,
    .min_width = 1,
    .max_width = 2,
    .spawn_chance = 0.4f,
    .biome_mask = 1u << BIOME_TYPE_DESERT
  },
};

static void qs_biome_trees(trees_sort_key_t *array, int left, int right){
  int i = left;
  int j = right;
  float pivot = array[(left + right) / 2].spawn_chance;

  while(i <= j){
    while(array[i].spawn_chance < pivot) i++;
    while(array[j].spawn_chance > pivot) j--;

    if(i <= j){
      trees_sort_key_t tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
      i++;
      j--;
    }
  }

  if(left < j)  qs_biome_trees(array, left, j);
  if(i < right) qs_biome_trees(array, i, right);
}

static void biome_trees_quicksort(trees_sort_key_t *array, int count){
  if(count > 1)
    qs_biome_trees(array, 0, count - 1);
}

static int32_t get_num_trees_in_biome(biome_type_t biome_type){
  int32_t count = 0;
  int32_t trees_array_size = sizeof(trees) / sizeof(tree_t);

  // convert from biome type to biome mask (used to tell which trees fit in which biome)
  uint32_t biome_mask = 1u << biome_type;

  for(int32_t i = 0; i < trees_array_size; i++){
    if(trees[i].biome_mask & biome_mask) count++;
  }

  return count;
}

void place_tree(int32_t z_row, int32_t x_col, tree_type_t tree_type){
  tree_t tree = trees[tree_type];
  plant_species_t plant_species = tree.plant_species;

  uint32_t seed = (uint32_t)(z_row * 73856093u) ^ (uint32_t)(x_col * 19349663u); // two prime numbers for hashing
  bool will_spawn = random_spawn_from_seed(seed, tree.spawn_chance);

  if(!will_spawn) return;

  // find first solid block along y
  int32_t first_solid_y = 0;
  for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
    block_t *block = &block_at(y, z_row, x_col);
    if(block_is_water(block) || block_is_lava(block)) return;
    if(block_is_tree_trunk(block)) return;
    if(block_is_soil(block) || block_is_stone(block)){
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
  switch(plant_species){
    case PLANT_SPECIES_TREE:
      for(int32_t y = tree_middle; y <= tree_top; y++){
        for(int32_t z = z_row - 6; z <= z_row + 6; z++){
          for(int32_t x = x_col - 6; x <= x_col + 6; x++){
            if(xorshift32() % 4 == 0){
              assign_block_material(y, z, x, tree.leaves_block_type);
            }
          }
        }
      }
      break;

    case PLANT_SPECIES_CACTUS:
      for(int32_t y = tree_bottom; y < tree_top - 1; y++){
        uint32_t rand = xorshift32();
        if(rand < 0.1f * UINT32_MAX){
          if(xorshift32() % 2 == 0)
            assign_block_material(y, z_row + trunk_width, x_col, tree.leaves_block_type);
          else
            assign_block_material(y, z_row, x_col + trunk_width, tree.leaves_block_type);
        }
      }
      break;
  }
  
  // trunk
  for(int32_t y = tree_bottom; y <= tree_top - 2; y++){
    int32_t width = trunk_width;
    for(int32_t z = z_row; z < z_row + width; z++){
      for(int32_t x = x_col; x < x_col + width; x++){
        assign_block_material(y, z, x, tree.trunk_block_type);
      }
    }
  }
}

// place trees in a biome:
//   go through all trees that would exist in this biome
//   if tree density > biome tree density, then a tree may be placed, based on spawn chance
void place_trees(){

  int8_t trees_plane[MAP_NUM_BLOCKS_Z][MAP_NUM_BLOCKS_X] = {0};

  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z += 4){
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x += 4){
      biome_t biome = biomes[block_at(0, z, x).type];
      float trees_density_threshold = biome.trees_density_threshold;
      float trees_noise_frequency   = biome.trees_frequency;

      float trees_density = fabsf(perlin_noise_3d((float)x * trees_noise_frequency, 420.0f, (float)z * trees_noise_frequency));
 
      // mark spot with tree
      if(trees_density > trees_density_threshold) trees_plane[z][x] = 1;
    }
  }

  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      biome_type_t biome_type = block_at(0, z, x).biome_type;

      int32_t num_trees_in_this_biome = get_num_trees_in_biome(biome_type);
      trees_sort_key_t biome_trees_sort_array[num_trees_in_this_biome]; // list of trees in this biome

      // copy the keys from the trees array into the biome trees sorting array
      uint32_t biome_mask = 1u << biome_type;
      int32_t main_trees_array_size = sizeof(trees) / sizeof(tree_t); // main trees array size
      int32_t biome_trees_sort_array_index = 0; // index into local list of biome trees
      for(int32_t tree_i = 0; tree_i < main_trees_array_size; tree_i++){ // loop main trees list
        // if tree belongs to this biome, copy into sorting list
        if(trees[tree_i].biome_mask & biome_mask){
          biome_trees_sort_array[biome_trees_sort_array_index].tree_type = (tree_type_t)tree_i;
          biome_trees_sort_array[biome_trees_sort_array_index].spawn_chance = trees[tree_i].spawn_chance;
          biome_trees_sort_array_index++;
        }
      }
      // sort the biome trees list by spawn_chance (lower values earlier)
      biome_trees_quicksort(biome_trees_sort_array, num_trees_in_this_biome); 
 
      // if this spot has a tree, then try planting each of the trees that belong to this biome
      // from rarest first, to most common last, according to the list sorted by spawn_chance
      if(trees_plane[z][x] == 1)
        for(int32_t biome_tree_i = 0; biome_tree_i < num_trees_in_this_biome; biome_tree_i++){ 
          place_tree(z, x, biome_trees_sort_array[biome_tree_i].tree_type);
        }
    }
  }
  
}