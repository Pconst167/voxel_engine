#include "biome.h"

biome_t biomes[] = {
  [BIOME_TYPE_GRASSLANDS] = {
    .trees_density_threshold      = 0.5f,
    .trees_frequency              = 0.1f,
    
    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 8
  },
  [BIOME_TYPE_FOREST] = {
    .trees_density_threshold = 0.3f,
    .trees_frequency = 0.05f,

    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 8
  },
  [BIOME_TYPE_RAIN_FOREST] = {
    .trees_density_threshold = 0.3f,
    .trees_frequency = 0.05f,

    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 8
  },
  [BIOME_TYPE_DESERT] = {
    .trees_density_threshold = 0.7f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 8
  },
  [BIOME_TYPE_SNOW] = {
    .trees_density_threshold = 0.4f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 8
  },
  [BIOME_TYPE_VOLCANIC] = {
    .trees_density_threshold = 0.9f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 20
  },
  [BIOME_TYPE_SAVANNAH] = {
    .trees_density_threshold = 0.5f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.06,
    .large_caves_noise_frequency  = 0.04,
    .small_hills_noise_frequency  = 0.03,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .max_surface_height           = 8
  }
};