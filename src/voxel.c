#include <stdio.h>
#include "voxel.h"

// Y slowest changing dimension, X is fastest changing
// this array has +2 elements in each index.  this is used for checking nearest neighbours
// when deciding whether to render a face.  without these extra elements there would be issues at
// the boundaries of the voxel world.  these elements therefore are always set as AIR type 
voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

material_t voxel_materials[4] = {
  {
    .material_name      = "air",
    .base_color         = (color_t){.uint32 = 0x00000000},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .two_sided_lighting = false,
    .reflects_light     = false,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .texture            = NULL
  },
  {
    .material_name      = "stone",
    .base_color         = (color_t){.uint32 = 0xFF808080},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .two_sided_lighting = false,
    .reflects_light     = true,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .texture            = NULL
  },
  {
    .material_name      = "dirt",
    .base_color         = (color_t){.uint32 = 0xFF408080},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .two_sided_lighting = false,
    .reflects_light     = true,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .texture            = NULL
  },
  {
    .material_name      = "grass",
    .base_color         = (color_t){.uint32 = 0xFF0FF000},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .two_sided_lighting = false,
    .reflects_light     = true,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .texture            = NULL
  }
};

void init_voxel_map(){
  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        if(y <= 2)
          voxels[y][z][x].type = VOXEL_TYPE_STONE;
        else if(y >= 3 && y <= 4)
          voxels[y][z][x].type = VOXEL_TYPE_DIRT;
        else if(y >= 5 && y <= 6)
          voxels[y][z][x].type = VOXEL_TYPE_GRASS;
        else
          voxels[y][z][x].type = VOXEL_TYPE_AIR;
        
        voxels[y][z][x].voxel_id = y * MAP_NUM_VOXELS_Z * MAP_NUM_VOXELS_X + z * MAP_NUM_VOXELS_X + x;
        voxels[y][z][x].aabb.x_min = x;
        voxels[y][z][x].aabb.x_max = x + 1.0;
        voxels[y][z][x].aabb.z_min = z;
        voxels[y][z][x].aabb.z_max = z + 1.0;
        voxels[y][z][x].aabb.y_min = y;
        voxels[y][z][x].aabb.y_max = y + 1.0;
      }
    }
  }

  voxel_materials[VOXEL_TYPE_STONE].texture = load_texture("./assets/textures/voxel/stone.png");
  voxel_materials[VOXEL_TYPE_DIRT].texture  = load_texture("./assets/textures/voxel/dirt.png");
  voxel_materials[VOXEL_TYPE_GRASS].texture = load_texture("./assets/textures/voxel/grass_top.png");
}

material_t get_voxel_material(uint32_t y_plane, uint32_t z_row, uint32_t x_col){
  return voxel_materials[voxels[y_plane][z_row][x_col].type];
}