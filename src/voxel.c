#include <stdio.h>
#include "voxel.h"
#include "misc.h"

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
        if(y <= 32){
          voxels[y][z][x].type = VOXEL_TYPE_STONE;
        }
        else if(y >= 33 && y <= 40){
          voxels[y][z][x].type = VOXEL_TYPE_DIRT;
        }
        else if(y == 41 || y == 42){
          if(xorshift32() % 5 == 0)
            voxels[y][z][x].type = VOXEL_TYPE_GRASS;
          else
            voxels[y][z][x].type = VOXEL_TYPE_AIR;
        }
        else{
          voxels[y][z][x].type = VOXEL_TYPE_AIR;
        }
        
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

void get_voxel_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z){
  *start_x = clamp_int((int)floorf(aabb->x_min), 0, MAP_NUM_VOXELS_X - 1);
  *end_x   = clamp_int((int)floorf(aabb->x_max), 0, MAP_NUM_VOXELS_X - 1);

  *start_y = clamp_int((int)floorf(aabb->y_min), 0, MAP_NUM_VOXELS_Y - 1);
  *end_y   = clamp_int((int)floorf(aabb->y_max), 0, MAP_NUM_VOXELS_Y - 1);

  *start_z = clamp_int((int)floorf(aabb->z_min), 0, MAP_NUM_VOXELS_Z - 1);
  *end_z   = clamp_int((int)floorf(aabb->z_max), 0, MAP_NUM_VOXELS_Z - 1);
}

inline voxel_quad_t generate_voxel_quad(voxel_quad_type_t quad_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col){
  voxel_quad_t quad;

  switch(quad_type){
    case VOXEL_QUAD_TOP:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane + 1.0f;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row + 1.0f;
      quad.points[2].x = x_col + 1.0f;
      quad.points[2].y = y_plane + 1.0f;
      quad.points[2].z = z_row;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_TOP;
      return quad;
      
    case VOXEL_QUAD_BOTTOM:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col + 1.0f;
      quad.points[1].y = y_plane;
      quad.points[1].z = z_row;
      quad.points[2].x = x_col;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row + 1.0f;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 1.0f;
      quad.uv[1].v = 0.0f;
      quad.uv[2].u = 0.0f;
      quad.uv[2].v = 1.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 0.0f;
      quad.uv[4].v = 1.0f;
      quad.uv[5].u = 1.0f;
      quad.uv[5].v = 0.0f;
      quad.quad_type = VOXEL_QUAD_BOTTOM;
    return quad;
    
    case VOXEL_QUAD_LEFT:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row + 1.0f;
      quad.points[1].x = x_col;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row + 1.0f;
      quad.points[2].x = x_col;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_LEFT;
      return quad;
      
    case VOXEL_QUAD_RIGHT:
      quad.points[0].x = x_col + 1.0f;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col + 1.0f;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row;
      quad.points[2].x = x_col + 1.0f;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row + 1.0f;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_RIGHT;
      return quad;
      
    case VOXEL_QUAD_BACK:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row;
      quad.points[2].x = x_col + 1.0f;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_BACK;
      return quad;
      
    case VOXEL_QUAD_FRONT:
    default:
      quad.points[0].x = x_col + 1.0f;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row + 1.0f;
      quad.points[1].x = x_col + 1.0f;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row + 1.0f;
      quad.points[2].x = x_col;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row + 1.0f;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_FRONT;
      return quad;
  }
}