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

typedef struct{
  uint32_t voxel_id;
  voxel_type_t type;
  aabb_3d_t aabb;
} voxel_t;

extern voxel_t voxels[MAP_NUM_VOXELS_X][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_Y];

void populate_voxel_map();

#endif