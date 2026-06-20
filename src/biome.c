#include "biome.h"

biome_t biomes[] = {
  [BIOME_TYPE_GRASSLANDS] = {
    .name = "grasslands",

    .trees_density_threshold      = 0.5f,
    .trees_frequency              = 0.1f,
    
    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  },
  [BIOME_TYPE_FOREST] = {
    .name = "forest",

    .trees_density_threshold = 0.3f,
    .trees_frequency = 0.05f,

    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  },
  [BIOME_TYPE_RAIN_FOREST] = {
    .name = "rain forest",

    .trees_density_threshold = 0.3f,
    .trees_frequency = 0.05f,

    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  },
  [BIOME_TYPE_DESERT] = {
    .name = "desert",

    .trees_density_threshold = 0.7f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  },
  [BIOME_TYPE_SNOW] = {
    .name = "snow",

    .trees_density_threshold = 0.4f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  },
  [BIOME_TYPE_VOLCANIC] = {
    .name = "volcanic",

    .trees_density_threshold = 0.5f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  },
  [BIOME_TYPE_SAVANNAH] = {
    .name = "savannah",

    .trees_density_threshold = 0.5f,
    .trees_frequency = 0.1f,

    .bed_lava_noise_frequency     = 0.1,
    .bed_lava_noise_amplitude     = 8,

    .small_caves_noise_frequency  = 0.08,
    .medium_caves_noise_frequency = 0.04,
    .large_caves_noise_frequency  = 0.02,
    .small_caves_noise_amplitude  = 16,
    .medium_caves_noise_amplitude = 32,
    .large_caves_noise_amplitude  = 64,

    .small_hills_noise_frequency  = 0.04,
    .medium_hills_noise_frequency = 0.02,
    .large_hills_noise_frequency  = 0.01,
    .small_hills_noise_amplitude  = 4,
    .medium_hills_noise_amplitude = 8,
    .large_hills_noise_amplitude  = 16,
  }
};