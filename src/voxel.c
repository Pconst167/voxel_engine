#include <stdio.h>
#include "voxel.h"

// Y slowest changing dimension, X is fastest changing
voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

voxel_material_t voxel_materials[4] = {
  {
    .material_name      = "air",
    .base_color         = (color_t){.uint32 = 0x00000000},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .reflects_light     = false,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .side_texture       = NULL,
    .top_texture        = NULL
  },
  {
    .material_name      = "stone",
    .base_color         = (color_t){.uint32 = 0xFF808080},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .reflects_light     = true,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .side_texture       = NULL,
    .top_texture        = NULL
  },
  {
    .material_name      = "dirt",
    .base_color         = (color_t){.uint32 = 0xFF408080},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .reflects_light     = true,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .side_texture       = NULL,
    .top_texture        = NULL
  },
  {
    .material_name      = "grass",
    .base_color         = (color_t){.uint32 = 0xFF0FF000},
    .emissive_color     = (color_t){.uint32 = 0x00000000},
    .opacity            = 1.0f,
    .reflects_light     = true,
    .dither             = false,
    .blend_mode         = BLEND_MODE_OPAQUE,
    .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
    .side_texture       = NULL,
    .top_texture        = NULL
  }
};

void init_voxel_map(){
  voxel_materials[VOXEL_TYPE_STONE].side_texture = load_texture("./assets/textures/voxel/stone.png");
  voxel_materials[VOXEL_TYPE_STONE].top_texture  = load_texture("./assets/textures/voxel/stone.png");
  voxel_materials[VOXEL_TYPE_DIRT].side_texture  = load_texture("./assets/textures/voxel/dirt.png");
  voxel_materials[VOXEL_TYPE_DIRT].top_texture   = load_texture("./assets/textures/voxel/dirt.png");
  voxel_materials[VOXEL_TYPE_GRASS].side_texture = load_texture("./assets/textures/voxel/grass_side.png");
  voxel_materials[VOXEL_TYPE_GRASS].top_texture  = load_texture("./assets/textures/voxel/grass_top.png");

  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        if(y <= 2){
          voxels[y][z][x].type = VOXEL_TYPE_STONE;
          voxels[y][z][x].voxel_material = &voxel_materials[VOXEL_TYPE_STONE];
        }
        else if(y >= 3 && y <= 5){
          voxels[y][z][x].type = VOXEL_TYPE_DIRT;
          voxels[y][z][x].voxel_material = &voxel_materials[VOXEL_TYPE_DIRT];
        }
        else if(y >= 6 && y <= 6){
          voxels[y][z][x].type = VOXEL_TYPE_GRASS;
          voxels[y][z][x].voxel_material = &voxel_materials[VOXEL_TYPE_GRASS];
        }
        else{
          voxels[y][z][x].type = VOXEL_TYPE_AIR;
          voxels[y][z][x].voxel_material = NULL;
        }
        
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
}