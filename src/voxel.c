#include "voxel.h"

voxel_t voxels[MAP_NUM_VOXELS_X][MAP_NUM_VOXELS_Z][MAP_NUM_VOXELS_Y];

void populate_voxel_map(){
  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        if(y == 0){
          voxels[x][z][y].type = VOXEL_TYPE_DIRT;
        }
        else{
          voxels[x][z][y].type = VOXEL_TYPE_AIR;
        
          voxels[x][z][y].voxel_id = y * MAP_NUM_VOXELS_Z * MAP_NUM_VOXELS_X + z * MAP_NUM_VOXELS_X + x;
          voxels[x][z][y].aabb.x_min = x;
          voxels[x][z][y].aabb.x_max = x + 1.0;
          voxels[x][z][y].aabb.y_min = y;
          voxels[x][z][y].aabb.y_max = y + 1.0;
          voxels[x][z][y].aabb.z_min = z;
          voxels[x][z][y].aabb.z_max = z + 1.0;
        }
      }
    }
  }
}