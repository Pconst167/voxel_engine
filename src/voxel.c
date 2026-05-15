#include <stdio.h>
#include <float.h>
#include "voxel.h"
#include "misc.h"
#include "camera.h"

// Y slowest changing dimension, X is fastest changing
voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

// voxel currently being looked at
voxel_raycast_hit_t looked_at_block;

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
        voxels[y][z][x].solid = false;

        if(y <= (uint32_t)(0.3 * MAP_NUM_VOXELS_Y)){
          voxels[y][z][x].type = VOXEL_TYPE_STONE;
          voxels[y][z][x].solid = true;
        }
        else if(y > (uint32_t)(0.3 * MAP_NUM_VOXELS_Y) && y < (uint32_t)(0.4 * MAP_NUM_VOXELS_Y)){
          voxels[y][z][x].type = VOXEL_TYPE_DIRT;
          voxels[y][z][x].solid = true;
        }
        else if(y == (uint32_t)(0.4 * MAP_NUM_VOXELS_Y)){
          if(xorshift32() % 5 == 0){
            voxels[y][z][x].type = VOXEL_TYPE_GRASS;
            voxels[y][z][x].solid = true;
          }
          else{
            voxels[y][z][x].type = VOXEL_TYPE_AIR;
          }
        }
        else{
          voxels[y][z][x].type = VOXEL_TYPE_AIR;
        }

        if((x <= 10 || x >= MAP_NUM_VOXELS_X - 10 || z <= 10 || z >= MAP_NUM_VOXELS_Z - 10) && y >= 0.4 * MAP_NUM_VOXELS_Y && y <= 0.43 * MAP_NUM_VOXELS_Y){
          if(xorshift32() % 5 == 0){
            voxels[y][z][x].type = VOXEL_TYPE_DIRT;
            voxels[y][z][x].solid = true;
          }
          else{
            voxels[y][z][x].type = VOXEL_TYPE_AIR;
          }
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

voxel_raycast_hit_t raycast_voxels(
  vector3_t ray_origin,
  vector3_t ray_direction,
  float max_distance
){
  voxel_raycast_hit_t hit_result = {0};

  int voxel_x = (int)floorf(ray_origin.x);
  int voxel_y = (int)floorf(ray_origin.y);
  int voxel_z = (int)floorf(ray_origin.z);

  int step_x = (ray_direction.x > 0.0f) ? 1 : -1;
  int step_y = (ray_direction.y > 0.0f) ? 1 : -1;
  int step_z = (ray_direction.z > 0.0f) ? 1 : -1;

  float next_boundary_x = (step_x > 0) ? (float)(voxel_x + 1) : (float)voxel_x;
  float next_boundary_y = (step_y > 0) ? (float)(voxel_y + 1) : (float)voxel_y;
  float next_boundary_z = (step_z > 0) ? (float)(voxel_z + 1) : (float)voxel_z;

  float t_max_x = (ray_direction.x != 0.0f) ? (next_boundary_x - ray_origin.x) / ray_direction.x : FLT_MAX;
  float t_max_y = (ray_direction.y != 0.0f) ? (next_boundary_y - ray_origin.y) / ray_direction.y : FLT_MAX;
  float t_max_z = (ray_direction.z != 0.0f) ? (next_boundary_z - ray_origin.z) / ray_direction.z : FLT_MAX;

  float t_delta_x = (ray_direction.x != 0.0f) ? fabsf(1.0f / ray_direction.x) : FLT_MAX;
  float t_delta_y = (ray_direction.y != 0.0f) ? fabsf(1.0f / ray_direction.y) : FLT_MAX;
  float t_delta_z = (ray_direction.z != 0.0f) ? fabsf(1.0f / ray_direction.z) : FLT_MAX;

  float current_distance = 0.0f;

  while(current_distance <= max_distance) {
    if(
      voxel_x >= 0 && voxel_x < MAP_NUM_VOXELS_X &&
      voxel_y >= 0 && voxel_y < MAP_NUM_VOXELS_Y &&
      voxel_z >= 0 && voxel_z < MAP_NUM_VOXELS_Z
    ) {
      if(voxels[voxel_y][voxel_z][voxel_x].solid) {
        hit_result.hit = 1;
        hit_result.voxel_x = voxel_x;
        hit_result.voxel_y = voxel_y;
        hit_result.voxel_z = voxel_z;
        hit_result.distance = current_distance;
        return hit_result;
      }
    }

    if(t_max_x < t_max_y && t_max_x < t_max_z) {
      voxel_x += step_x;
      current_distance = t_max_x;
      t_max_x += t_delta_x;

      hit_result.normal_x = -step_x;
      hit_result.normal_y = 0;
      hit_result.normal_z = 0;
    }
    else if(t_max_y < t_max_z) {
      voxel_y += step_y;
      current_distance = t_max_y;
      t_max_y += t_delta_y;

      hit_result.normal_x = 0;
      hit_result.normal_y = -step_y;
      hit_result.normal_z = 0;
    }
    else {
      voxel_z += step_z;
      current_distance = t_max_z;
      t_max_z += t_delta_z;

      hit_result.normal_x = 0;
      hit_result.normal_y = 0;
      hit_result.normal_z = -step_z;
    }
  }

  return hit_result;
}

void calculate_currently_looked_at_voxel(){
  looked_at_block = raycast_voxels(
    camera.position,
    camera.direction,
    MAX_VOXEL_LOOK_AT_DISTANCE
  );
}