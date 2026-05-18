#ifndef VOXEL_H
#define VOXEL_H

#include "aabb.h"
#include "material.h"
#include "mesh.h"
#include "light.h"

#define MAP_NUM_VOXELS_X 512
#define MAP_NUM_VOXELS_Z 512
#define MAP_NUM_VOXELS_Y 256
#define CHUNK_SIZE 16
#define NUM_CHUNK_DIVISIONS_Y (MAP_NUM_VOXELS_Y / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_Z (MAP_NUM_VOXELS_Z / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_X (MAP_NUM_VOXELS_X / CHUNK_SIZE)
#define CHUNK_RENDER_DISTANCE 5 // render this number of chunks around the player in XZ

#define MAX_VOXEL_LOOK_AT_DISTANCE 6.0f
#define BLOCK_TYPE_STRING_NAME_MAX_LEN 32

// macro to get block at coordiates x y z
#define MAP_X_STRIDE 1
#define MAP_Z_STRIDE MAP_NUM_VOXELS_X
#define MAP_Y_STRIDE (MAP_NUM_VOXELS_X * MAP_NUM_VOXELS_Z)
#define block_at(y, z, x) blocks[(y) * MAP_Y_STRIDE + (z) * MAP_Z_STRIDE + (x)]

typedef enum{
  BLOCK_TYPE_AIR          = 0,
  BLOCK_TYPE_STONE        = 1,
  BLOCK_TYPE_DIRT         = 2,
  BLOCK_TYPE_GRASS        = 3,
  BLOCK_TYPE_TREE_TRUNK0  = 4,
  BLOCK_TYPE_TREE_TRUNK1  = 5,
  BLOCK_TYPE_TREE_TRUNK2  = 6,
  BLOCK_TYPE_TREE_TRUNK3  = 7,
  BLOCK_TYPE_TREE_TRUNK4  = 8,
  BLOCK_TYPE_TREE_TRUNK5  = 9,
  BLOCK_TYPE_TREE_TRUNK6  = 10,
  BLOCK_TYPE_TREE_TRUNK7  = 11,
  BLOCK_TYPE_LEAVES       = 12,
  BLOCK_TYPE_GLASS        = 13,
} block_type_t;

typedef enum{
  VOXEL_QUAD_TOP,
  VOXEL_QUAD_BOTTOM,
  VOXEL_QUAD_LEFT,
  VOXEL_QUAD_RIGHT,
  VOXEL_QUAD_FRONT,
  VOXEL_QUAD_BACK,
} block_quad_type_t;

typedef struct{
  bool backface_culling;
  // color and light
  color_t base_color;
  color_t emissive_color;   //  object's own emission color */
  float opacity;            // used as a global alpha blending - makes object totally opaque or ghostly */
  bool reflects_light;
  bool dither;              // individual faces can be dithered
  // alpha
  blend_mode_t blend_mode;
  uint8_t alpha_cutoff;
  // texture data
  texture_t *side_texture;
  texture_t *top_texture;
} block_material_t;

typedef struct{
  block_quad_type_t quad_type;
  vector3_t points[4];
  uv_t uv[6];
  light_intensity_rgb_t total_light_intensity;
} block_quad_t;

typedef struct{
  block_type_t type;
  bool solid;
  aabb_3d_t aabb;
  block_material_t *block_material;
  float avg_w;
  bool looked_at;
} block_t;

typedef struct{
  int32_t hit;
  int32_t block_x;
  int32_t block_y;
  int32_t block_z;
  int32_t normal_x;
  int32_t normal_y;
  int32_t normal_z;
  float distance;
} block_raycast_hit_t;

// keep block type names as strings
extern char *block_type_names[];

extern block_material_t *block_materials;

// Y slowest changing dimension, X is fastest changing
extern block_t *blocks;

// block currently being looked at
extern block_raycast_hit_t looked_at_block;

void init_block_materials();
void init_block_map();
void get_block_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z);
block_quad_t generate_block_quad(block_quad_type_t quad_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col);

block_raycast_hit_t raycast_blocks(vector3_t ray_origin, vector3_t ray_direction, float max_distance);
void calculate_currently_looked_at_block();
void assign_block_material(int32_t block_y, int32_t block_z, int32_t block_x, block_type_t block_type);

char *block_type_to_string(block_type_t block_type);

#endif