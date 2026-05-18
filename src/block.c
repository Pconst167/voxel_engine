#include <stdio.h>
#include <float.h>
#include <stdlib.h>
#include "block.h"
#include "misc.h"
#include "camera.h"
#include "array.h"
#include "trees.h"
#include "log.h"

// Y slowest changing dimension, X is fastest changing
block_t *blocks;

// block currently being looked at
block_raycast_hit_t looked_at_block;

block_material_t *block_materials;

void init_block_materials(){
  array_push(
    block_materials, 
    ((block_material_t){
      .backface_culling   = true,
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
    block_materials, 
    ((block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/stone2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone2.png")
    })
  );

  array_push(
    block_materials, 
    ((block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/dirt_2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/dirt_2.png")
    })
  );

  array_push(
    block_materials,
    ((block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/grass_side_2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/grass_top.png")   
    })
  );

  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk0_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk0_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk1_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk1_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk2_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk2_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk3_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk3_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk4_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk4_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk5_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk5_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk6_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk6_top.png")   
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/treetrunk7_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk7_top.png")   
    }
  );

  array_push(
    block_materials,
    ((block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/tree_leaves.png"),
      .top_texture        = load_texture("./assets/textures/blocks/tree_leaves.png")   
    })
  );

  array_push(
    block_materials,
    ((block_material_t){
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_ALPHA_TEST,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .side_texture       = load_texture("./assets/textures/blocks/glass0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/glass0.png")   
    })
  );
}

char *block_type_names[] = {
  "air",
  "stone",
  "dirt",
  "grass",
  "treetrunk0",
  "treetrunk1",
  "treetrunk2",
  "treetrunk3",
  "treetrunk4",
  "treetrunk5",
  "treetrunk6",
  "treetrunk7",
  "leaves",
  "glass"
};

void init_block_map(){

  blocks = malloc(sizeof(block_t) * MAP_NUM_VOXELS_Y * MAP_NUM_VOXELS_Z * MAP_NUM_VOXELS_X);
  if(blocks == NULL){
    print_log("ERROR: unable to allocate map memory");
    exit(1);
  }

  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        block_at(y, z, x).solid = false;
        assign_block_material(y, z, x, BLOCK_TYPE_AIR);

        if(y <= (uint32_t)(0.3 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_STONE);
          block_at(y, z, x).solid = true;
        }
        else if(y > (uint32_t)(0.3 * MAP_NUM_VOXELS_Y) && y < (uint32_t)(0.4 * MAP_NUM_VOXELS_Y) - 1){
          assign_block_material(y, z, x, BLOCK_TYPE_DIRT);
          block_at(y, z, x).solid = true;
        }
        else if(y == (uint32_t)(0.4 * MAP_NUM_VOXELS_Y) - 1){
          if(xorshift32() % 5 == 0){
            assign_block_material(y, z, x, BLOCK_TYPE_DIRT);
          }
          else{
            assign_block_material(y, z, x, BLOCK_TYPE_GRASS);
          }
          block_at(y, z, x).solid = true;
        }
        else if(y == (uint32_t)(0.4 * MAP_NUM_VOXELS_Y)){
          if(xorshift32() % 5 == 0){
            uint32_t random = xorshift32();
            if(random % 2 == 0){
              assign_block_material(y, z, x, BLOCK_TYPE_GRASS);
              block_at(y, z, x).solid = true;
            }
            else{
              assign_block_material(y, z, x, BLOCK_TYPE_LEAVES);
              block_at(y, z, x).solid = true;
            }
          }
        }

        if((x <= 10 || x >= MAP_NUM_VOXELS_X - 10 || z <= 10 || z >= MAP_NUM_VOXELS_Z - 10) && y >= 0.4 * MAP_NUM_VOXELS_Y && y <= 0.43 * MAP_NUM_VOXELS_Y){
          if(xorshift32() % 5 == 0){
            assign_block_material(y, z, x, BLOCK_TYPE_LEAVES);
            block_at(y, z, x).solid = true;
          }
          else{
            block_at(y, z, x).type = BLOCK_TYPE_AIR;
          }
        }

        block_at(y, z, x).aabb.x_min = x;
        block_at(y, z, x).aabb.x_max = x + 1.0;
        block_at(y, z, x).aabb.z_min = z;
        block_at(y, z, x).aabb.z_max = z + 1.0;
        block_at(y, z, x).aabb.y_min = y;
        block_at(y, z, x).aabb.y_max = y + 1.0;
      }
    }
  }

  for(uint32_t z = 32; z < MAP_NUM_VOXELS_Z - 32; z++){
    for(uint32_t x = 32; x < MAP_NUM_VOXELS_X - 32; x++){
      if(x % 27 == 0 && z % 23 == 0){
        int32_t rand = xorshift32();
        if(rand % 7 == 0)
          place_tree(z, x, TREE0);
        else if(rand % 7 == 1)
          place_tree(z, x, TREE1);
        else if(rand % 7 == 2)
          place_tree(z, x, TREE2);
        else if(rand % 7 == 3)
          place_tree(z, x, TREE3);
        else if(rand % 7 == 4)
          place_tree(z, x, TREE4);
        else if(rand % 7 == 5)
          place_tree(z, x, TREE5);
        else if(rand % 7 == 6)
          place_tree(z, x, TREE6);
        else
          place_tree(z, x, TREE7);
      }
    }
  }
}

void get_block_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z){
  *start_x = clamp_int((int)floorf(aabb->x_min), 0, MAP_NUM_VOXELS_X - 1);
  *end_x   = clamp_int((int)floorf(aabb->x_max), 0, MAP_NUM_VOXELS_X - 1);

  *start_y = clamp_int((int)floorf(aabb->y_min), 0, MAP_NUM_VOXELS_Y - 1);
  *end_y   = clamp_int((int)floorf(aabb->y_max), 0, MAP_NUM_VOXELS_Y - 1);

  *start_z = clamp_int((int)floorf(aabb->z_min), 0, MAP_NUM_VOXELS_Z - 1);
  *end_z   = clamp_int((int)floorf(aabb->z_max), 0, MAP_NUM_VOXELS_Z - 1);
}

inline block_quad_t generate_block_quad(block_quad_type_t quad_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col){
  block_quad_t quad;

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

block_raycast_hit_t raycast_blocks(
  vector3_t ray_origin,
  vector3_t ray_direction,
  float max_distance
){
  block_raycast_hit_t hit_result = {0};

  int block_x = (int)floorf(ray_origin.x);
  int block_y = (int)floorf(ray_origin.y);
  int block_z = (int)floorf(ray_origin.z);

  int step_x = (ray_direction.x > 0.0f) ? 1 : -1;
  int step_y = (ray_direction.y > 0.0f) ? 1 : -1;
  int step_z = (ray_direction.z > 0.0f) ? 1 : -1;

  float next_boundary_x = (step_x > 0) ? (float)(block_x + 1) : (float)block_x;
  float next_boundary_y = (step_y > 0) ? (float)(block_y + 1) : (float)block_y;
  float next_boundary_z = (step_z > 0) ? (float)(block_z + 1) : (float)block_z;

  float t_max_x = (ray_direction.x != 0.0f) ? (next_boundary_x - ray_origin.x) / ray_direction.x : FLT_MAX;
  float t_max_y = (ray_direction.y != 0.0f) ? (next_boundary_y - ray_origin.y) / ray_direction.y : FLT_MAX;
  float t_max_z = (ray_direction.z != 0.0f) ? (next_boundary_z - ray_origin.z) / ray_direction.z : FLT_MAX;

  float t_delta_x = (ray_direction.x != 0.0f) ? fabsf(1.0f / ray_direction.x) : FLT_MAX;
  float t_delta_y = (ray_direction.y != 0.0f) ? fabsf(1.0f / ray_direction.y) : FLT_MAX;
  float t_delta_z = (ray_direction.z != 0.0f) ? fabsf(1.0f / ray_direction.z) : FLT_MAX;

  float current_distance = 0.0f;

  while(current_distance <= max_distance){
    if(block_x >= 0 && block_x < MAP_NUM_VOXELS_X &&
       block_y >= 0 && block_y < MAP_NUM_VOXELS_Y &&
       block_z >= 0 && block_z < MAP_NUM_VOXELS_Z
    ){
      if(block_at(block_y, block_z, block_x).solid){
        hit_result.hit = 1;
        hit_result.block_x = block_x;
        hit_result.block_y = block_y;
        hit_result.block_z = block_z;
        hit_result.distance = current_distance;
        return hit_result;
      }
    }

    if(t_max_x < t_max_y && t_max_x < t_max_z){
      block_x += step_x;
      current_distance = t_max_x;
      t_max_x += t_delta_x;

      hit_result.normal_x = -step_x;
      hit_result.normal_y = 0;
      hit_result.normal_z = 0;
    }
    else if(t_max_y < t_max_z){
      block_y += step_y;
      current_distance = t_max_y;
      t_max_y += t_delta_y;

      hit_result.normal_x = 0;
      hit_result.normal_y = -step_y;
      hit_result.normal_z = 0;
    }
    else{
      block_z += step_z;
      current_distance = t_max_z;
      t_max_z += t_delta_z;

      hit_result.normal_x = 0;
      hit_result.normal_y = 0;
      hit_result.normal_z = -step_z;
    }
  }

  return hit_result;
}

void calculate_currently_looked_at_block(){
  looked_at_block = raycast_blocks(
    camera.position,
    camera.direction,
    MAX_VOXEL_LOOK_AT_DISTANCE
  );
}

void assign_block_material(int32_t block_y, int32_t block_z, int32_t block_x, block_type_t block_type){
  block_at(block_y, block_z, block_x).type = block_type;
  block_at(block_y, block_z, block_x).block_material = &block_materials[block_type];
}

char *block_type_to_string(block_type_t block_type){
  return block_type_names[block_type];
}