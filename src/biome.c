#include "biome.h"

biome_t biomes[] = {
  [BIOME_TYPE_GRASSLANDS] = {
    .trees_density_threshold = 0.5f,
    .trees_frequency = 0.1f,
  },
  [BIOME_TYPE_FOREST] = {
    .trees_density_threshold = 0.3f,
    .trees_frequency = 0.05f,
  },
  [BIOME_TYPE_RAIN_FOREST] = {
    .trees_density_threshold = 0.3f,
    .trees_frequency = 0.05f,
  },
  [BIOME_TYPE_DESERT] = {
    .trees_density_threshold = 0.7f,
    .trees_frequency = 0.1f,
  },
  [BIOME_TYPE_SNOW] = {
    .trees_density_threshold = 0.4f,
    .trees_frequency = 0.1f,
  },
  [BIOME_TYPE_VOLCANIC] = {
    .trees_density_threshold = 0.9f,
    .trees_frequency = 0.1f,
  },
  [BIOME_TYPE_SAVANNAH] = {
    .trees_density_threshold = 0.5f,
    .trees_frequency = 0.1f,
  }
};