#ifndef VOXEL_H
#define VOXEL_H

#include "aabb.h"

#define MAP_NUM_VOXELS_X 128
#define MAP_NUM_VOXELS_Z 128
#define MAP_NUM_VOXELS_Y 16
#define CHUNK_SIZE 16
#define NUM_CHUNK_DIVISIONS_X (MAP_NUM_VOXELS_X / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_Z (MAP_NUM_VOXELS_Z / CHUNK_SIZE)

typedef enum{
  VOXEL_TYPE_AIR,
  VOXEL_TYPE_DIRT,
  VOXEL_TYPE_STONE
} voxel_type_t;

typedef enum{
  VOXEL_FACE_TOP_LEFT,
  VOXEL_FACE_TOP_RIGHT,
  VOXEL_FACE_BOTTON_LEFT,
  VOXEL_FACE_BOTTOM_RIGHT,
  VOXEL_FACE_LEFT_LEFT,
  VOXEL_FACE_LEFT_RIGHT,
  VOXEL_FACE_RIGHT_LEFT,
  VOXEL_FACE_RIGHT_RIGHT,
  VOXEL_FACE_FRONT_LEFT,
  VOXEL_FACE_FRONT_RIGHT,
  VOXEL_FACE_BACK_LEFT,
  VOXEL_FACE_BACK_RIGHT
} voxel_face_t;

typedef struct{
  uint32_t voxel_id;
  voxel_type_t type;
  aabb_3d_t aabb;
} voxel_t;

// Y slowest changing dimension, X is fastest changing
extern voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

void populate_voxel_map();

#endif