#ifndef VOXEL_H
#define VOXEL_H

#include "aabb.h"
#include "material.h"
#include "mesh.h"
#include "light.h"

#define MAP_NUM_VOXELS_X 128
#define MAP_NUM_VOXELS_Z 128
#define MAP_NUM_VOXELS_Y 32
#define CHUNK_SIZE 16
#define NUM_CHUNK_DIVISIONS_X (MAP_NUM_VOXELS_X / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_Z (MAP_NUM_VOXELS_Z / CHUNK_SIZE)

typedef enum{
  VOXEL_TYPE_AIR   = 0,
  VOXEL_TYPE_STONE = 1,
  VOXEL_TYPE_DIRT  = 2,
  VOXEL_TYPE_GRASS = 3,
} voxel_type_t;

typedef enum{
  VOXEL_QUAD_TOP,
  VOXEL_QUAD_BOTTOM,
  VOXEL_QUAD_LEFT,
  VOXEL_QUAD_RIGHT,
  VOXEL_QUAD_FRONT,
  VOXEL_QUAD_BACK,
} voxel_quad_type_t;

typedef struct{
  char material_name[16];
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
} voxel_material_t;

typedef struct{
  voxel_quad_type_t quad_type;
  vector3_t points[4];
  uv_t uv[6];
  light_intensity_rgb_t total_light_intensity;
} voxel_quad_t;

typedef struct{
  uint32_t voxel_id;
  voxel_type_t type;
  aabb_3d_t aabb;
  voxel_material_t *voxel_material;
  float avg_w;
} voxel_t;

extern voxel_material_t voxel_materials[4];

// Y slowest changing dimension, X is fastest changing
extern voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

void init_voxel_map();

#endif