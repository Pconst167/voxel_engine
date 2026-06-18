#ifndef BIOME_H
#define BIOME_H

#include <stdint.h>

#define WORLD_TEMPERATURE_HUMIDITY_SCALE 0.015

typedef enum{
  BIOME_TYPE_GRASSLANDS,
  BIOME_TYPE_FOREST,
  BIOME_TYPE_RAIN_FOREST,
  BIOME_TYPE_DESERT,
  BIOME_TYPE_SNOW,
  BIOME_TYPE_VOLCANIC,
  BIOME_TYPE_SAVANNAH
} biome_type_t;

typedef struct{
  float trees_density_threshold;
  float trees_frequency;

  float medium_caves_noise_frequency;
  float small_caves_noise_frequency;
  float large_caves_noise_frequency;

  float small_hills_noise_frequency;
  float large_hills_noise_frequency;
  float medium_hills_noise_frequency;
  float max_surface_height;

  float bed_lava_noise_frequency;
} biome_t;

extern biome_t biomes[];

#endif