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
#define CHUNK_RENDER_DISTANCE 4 // render this number of chunks around the player in XZ

#define MAX_VOXEL_LOOK_AT_DISTANCE 6.0f
#define BLOCK_TYPE_STRING_NAME_MAX_LEN 32

// macro to get block at coordiates x y z
#define MAP_X_STRIDE 1
#define MAP_Z_STRIDE MAP_NUM_VOXELS_X
#define MAP_Y_STRIDE (MAP_NUM_VOXELS_X * MAP_NUM_VOXELS_Z)
#define block_at(y, z, x) blocks[(y) * MAP_Y_STRIDE + (z) * MAP_Z_STRIDE + (x)]

typedef enum{
  BLOCK_TYPE_AIR        ,
  BLOCK_TYPE_LAVA0      ,
  BLOCK_TYPE_LAVA1      ,
  BLOCK_TYPE_VOLCANIC0  ,
  BLOCK_TYPE_VOLCANIC1  ,
  BLOCK_TYPE_VOLCANIC2  ,
  BLOCK_TYPE_VOLCANIC3  ,
  BLOCK_TYPE_STONE0     ,
  BLOCK_TYPE_STONE1     ,
  BLOCK_TYPE_STONE2     ,
  BLOCK_TYPE_STONE3     ,
  BLOCK_TYPE_STONE4     ,
  BLOCK_TYPE_STONE5     ,
  BLOCK_TYPE_STONE6     ,
  BLOCK_TYPE_STONE7     ,
  BLOCK_TYPE_STONE8     ,
  BLOCK_TYPE_COAL       ,
  BLOCK_TYPE_IRON       ,
  BLOCK_TYPE_DIAMOND    ,
  BLOCK_TYPE_SILVER     ,
  BLOCK_TYPE_GOLD       ,
  BLOCK_TYPE_DIRT       ,
  BLOCK_TYPE_GRASS      ,
  BLOCK_TYPE_TREE_TRUNK0,
  BLOCK_TYPE_TREE_TRUNK1,
  BLOCK_TYPE_TREE_TRUNK2,
  BLOCK_TYPE_TREE_TRUNK3,
  BLOCK_TYPE_TREE_TRUNK4,
  BLOCK_TYPE_TREE_TRUNK5,
  BLOCK_TYPE_TREE_TRUNK6,
  BLOCK_TYPE_TREE_TRUNK7,
  BLOCK_TYPE_LEAVES0    ,
  BLOCK_TYPE_LEAVES1    ,
  BLOCK_TYPE_LEAVES2    ,
  BLOCK_TYPE_LEAVES3    ,
  BLOCK_TYPE_LEAVES4    ,
  BLOCK_TYPE_LEAVES5    ,
  BLOCK_TYPE_LEAVES6    ,
  BLOCK_TYPE_LEAVES7    ,
  BLOCK_TYPE_LEAVES8    ,
  BLOCK_TYPE_LEAVES9    ,
  BLOCK_TYPE_FLOWER_LEAVES0,
  BLOCK_TYPE_FLOWER_LEAVES1,
  BLOCK_TYPE_GLASS      ,
  BLOCK_TYPE_LAMP0      ,
  BLOCK_TYPE_OVEN0      ,
  BLOCK_TYPE_CHEST0     ,
} block_type_t;

typedef enum{
  BLOCK_ORIENTATION_LEFT,
  BLOCK_ORIENTATION_RIGHT,
  BLOCK_ORIENTATION_FRONT,
  BLOCK_ORIENTATION_BACK
} block_orientation_t;

typedef enum{
  VOXEL_QUAD_TOP,
  VOXEL_QUAD_BOTTOM,
  VOXEL_QUAD_LEFT,
  VOXEL_QUAD_RIGHT,
  VOXEL_QUAD_FRONT,
  VOXEL_QUAD_BACK,
} block_quad_type_t;

typedef struct{
  char *material_name;
  // physical properties
  uint8_t mining_amount;
  // rendering
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
  texture_t *front_texture;
  texture_t *back_texture;
  texture_t *left_texture;
  texture_t *right_texture;
  texture_t *top_texture;
  texture_t *bottom_texture;
} block_material_t;

typedef struct{
  block_quad_type_t quad_type;
  vector3_t points[4];
  uv_t uv[6];
  light_intensity_rgb_t total_light_intensity;
} block_quad_t;

typedef struct{
  block_type_t type;
  block_orientation_t orientation;
  bool solid;
  uint8_t mining_amount;
  float temperature;
  bool mineable;
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

void set_block_orientation(int32_t block_y, int32_t block_z, int32_t block_x, vector3_t player_position);

char *get_block_material_name(block_type_t block_type);

#endif