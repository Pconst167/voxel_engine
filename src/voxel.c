#include "voxel.h"

// Y slowest changing dimension, X is fastest changing
// this array has +2 elements in each index.  this is used for checking nearest neighbours
// when deciding whether to render a face.  without these extra elements there would be issues at
// the boundaries of the voxel world.  these elements therefore are always set as AIR type 
voxel_t voxels[MAP_NUM_VOXELS_Y][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_X];

void populate_voxel_map(){
  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        if(y == 0 || y == 1 || y == 2 || (x == 0 && y <= 4) || (x == MAP_NUM_VOXELS_X-1 && y <= 4) || (z == 0 && y <= 4) || (z == MAP_NUM_VOXELS_Z-1 && y <= 4))
        {
          voxels[y][z][x].type = VOXEL_TYPE_DIRT;
        }
        else
        {
          voxels[y][z][x].type = VOXEL_TYPE_AIR;
        
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
}