#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include "voxel.h"
#include "misc.h"
#include "camera.h"
#include "array.h"
#include "log.h"

// Y slowest changing dimension, X is fastest changing
voxel_t *voxels;

// voxel currently being looked at
voxel_raycast_hit_t looked_at_block;

voxel_material_t *voxel_materials;

void init_block_materials(){
  array_push(
    voxel_materials, 
    ((voxel_material_t){
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = false,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = NULL,
      .top_texture        = NULL
    })  
  );

  array_push(
    voxel_materials, 
    ((voxel_material_t){
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/voxel/stone2.png"),
      .top_texture        = load_texture("./assets/textures/voxel/stone2.png")
    })
  );

  array_push(
    voxel_materials, 
    ((voxel_material_t){
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/voxel/dirt_2.png"),
      .top_texture        = load_texture("./assets/textures/voxel/dirt_2.png")
    })
  );

  array_push(
    voxel_materials,
    ((voxel_material_t){
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/voxel/grass_side_2.png"),
      .top_texture        = load_texture("./assets/textures/voxel/grass_top.png")   
    })
  );

  array_push(
    voxel_materials,
    ((voxel_material_t){
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/voxel/wood_side.png"),
      .top_texture        = load_texture("./assets/textures/voxel/wood_top.png")   
    })
  );

  array_push(
    voxel_materials,
    ((voxel_material_t){
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/voxel/tree_leaves.png"),
      .top_texture        = load_texture("./assets/textures/voxel/tree_leaves.png")   
    })
  );
}

char *block_type_names[] = {
  "air",
  "stone",
  "dirt",
  "grass",
  "wood",
  "leaves"
};

void place_tree(int32_t z_row, int32_t x_col, int32_t height){
  int32_t first_solid_y = 0;
  // find first solid block along y
  for(int32_t y = MAP_NUM_VOXELS_Y - 1; y >= 0; y--){
    if(voxel_at(y, z_row, x_col).solid){
      first_solid_y = y;
      break;
    }
  }
  
  int32_t tree_bottom = first_solid_y - 3; // -3 to place trunk below ground
  int32_t tree_top    = first_solid_y + 1 + height - 1;
  int32_t tree_middle = (tree_bottom + tree_top) / 2;

  // leaves
  for(int32_t y = tree_middle; y <= tree_top; y++){
    for(int32_t z = z_row - 3; z <= z_row + 3; z++){
      for(int32_t x = x_col - 3; x <= x_col + 3; x++){
        if(xorshift32() % 2 == 0){
          assign_block_material(y, z, x, VOXEL_TYPE_TREE_LEAVES);
          voxel_at(y, z, x).solid = true;
        }
        else{
          assign_block_material(y, z, x, VOXEL_TYPE_AIR);
          voxel_at(y, z, x).solid = false;
        }
      }
    }
  }

  // trunk
  for(int32_t y  = tree_bottom; y <= tree_top; y++){
    assign_block_material(y, z_row, x_col, VOXEL_TYPE_TREE_TRUNK);
    voxel_at(y, z_row, x_col).solid = true;
  }
}

void init_voxel_map(){

  voxels = malloc(sizeof(voxel_t) * MAP_NUM_VOXELS_Y * MAP_NUM_VOXELS_Z * MAP_NUM_VOXELS_X);
  if(voxels == NULL){
    print_log("ERROR: unable to allocate map memory");
    exit(1);
  }

  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        voxel_at(y, z, x).solid = false;
        assign_block_material(y, z, x, VOXEL_TYPE_AIR);

        if(y <= (uint32_t)(0.3 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, VOXEL_TYPE_STONE);
          voxel_at(y, z, x).solid = true;
        }
        else if(y > (uint32_t)(0.3 * MAP_NUM_VOXELS_Y) && y < (uint32_t)(0.4 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, VOXEL_TYPE_DIRT);
          voxel_at(y, z, x).solid = true;
        }
        else if(y == (uint32_t)(0.4 * MAP_NUM_VOXELS_Y)){
          if(xorshift32() % 5 == 0){
            assign_block_material(y, z, x, VOXEL_TYPE_GRASS);
            voxel_at(y, z, x).solid = true;
          }
        }

        if((x <= 10 || x >= MAP_NUM_VOXELS_X - 10 || z <= 10 || z >= MAP_NUM_VOXELS_Z - 10) && y >= 0.4 * MAP_NUM_VOXELS_Y && y <= 0.43 * MAP_NUM_VOXELS_Y){
          if(xorshift32() % 5 == 0){
            assign_block_material(y, z, x, VOXEL_TYPE_TREE_LEAVES);
            voxel_at(y, z, x).solid = true;
          }
          else{
            voxel_at(y, z, x).type = VOXEL_TYPE_AIR;
          }
        }

        voxel_at(y, z, x).aabb.x_min = x;
        voxel_at(y, z, x).aabb.x_max = x + 1.0;
        voxel_at(y, z, x).aabb.z_min = z;
        voxel_at(y, z, x).aabb.z_max = z + 1.0;
        voxel_at(y, z, x).aabb.y_min = y;
        voxel_at(y, z, x).aabb.y_max = y + 1.0;
      }
    }
  }

  for(uint32_t z = 32; z < MAP_NUM_VOXELS_Z - 32; z++){
    for(uint32_t x = 32; x < MAP_NUM_VOXELS_X - 32; x++){
      if(x % 27 == 0 && z % 23 == 0){
        int32_t rand = xorshift32();
        if(rand % 2 == 0)
          place_tree(z, x, 10);
        else if(rand % 2 == 1)
          place_tree(z, x, 15);
        else
          place_tree(z, x, 20);
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

  while(current_distance <= max_distance){
    if(voxel_x >= 0 && voxel_x < MAP_NUM_VOXELS_X &&
       voxel_y >= 0 && voxel_y < MAP_NUM_VOXELS_Y &&
       voxel_z >= 0 && voxel_z < MAP_NUM_VOXELS_Z
    ){
      if(voxel_at(voxel_y, voxel_z, voxel_x).solid){
        hit_result.hit = 1;
        hit_result.voxel_x = voxel_x;
        hit_result.voxel_y = voxel_y;
        hit_result.voxel_z = voxel_z;
        hit_result.distance = current_distance;
        return hit_result;
      }
    }

    if(t_max_x < t_max_y && t_max_x < t_max_z){
      voxel_x += step_x;
      current_distance = t_max_x;
      t_max_x += t_delta_x;

      hit_result.normal_x = -step_x;
      hit_result.normal_y = 0;
      hit_result.normal_z = 0;
    }
    else if(t_max_y < t_max_z){
      voxel_y += step_y;
      current_distance = t_max_y;
      t_max_y += t_delta_y;

      hit_result.normal_x = 0;
      hit_result.normal_y = -step_y;
      hit_result.normal_z = 0;
    }
    else{
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

void assign_block_material(int32_t block_y, int32_t block_z, int32_t block_x, voxel_type_t block_type){
  voxel_at(block_y, block_z, block_x).type = block_type;
  voxel_at(block_y, block_z, block_x).voxel_material = &voxel_materials[block_type];
}

char *block_type_to_string(voxel_type_t block_type){
  return block_type_names[block_type];
}