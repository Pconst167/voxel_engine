#include "chunk.h"
#include "array.h"
#include "log.h"
#include "debug.h"

chunk_t chunks[NUM_CHUNK_DIVISIONS_Z][NUM_CHUNK_DIVISIONS_X];

double world_x_min;
double world_x_max;
double world_z_min;
double world_z_max;
double world_y_min;
double world_y_max;

double chunk_dx;
double chunk_dz;
double chunk_dy;

/*
            /----/----/----/----/               /----/
           /    /    /    /    /      .        /    / 
          /----/----/----/----/      .        /----/
         /    /    /    /    /      .        /    /
        /----/----/----/----/      .        /----/
       /    /    /    /    /      .        /    /           10x10
      /----/----/----/----/      .        /----/
                              .....      
    /----/----/----/----/      .        /----/
   /    /    /    /    /      .        /    /
  /----/----/----/----/               /----/

*/

void calculate_world_bounds(void){
  /* set world boundaries */
  world_x_min = 0;
  world_x_max = MAP_NUM_VOXELS_X;
  world_z_min = 0;
  world_z_max = MAP_NUM_VOXELS_Z;
  world_y_min = 0;
  world_y_max = MAP_NUM_VOXELS_Y;
}

/* split the world into chunks */
void create_world_chunks(void){

  chunk_dx = CHUNK_SIZE;
  chunk_dz = CHUNK_SIZE;
  chunk_dy = CHUNK_SIZE;

  /* set the boundary of each chunk */
  for(uint32_t z_row = 0; z_row < NUM_CHUNK_DIVISIONS_Z; z_row++){
    for(uint32_t x_col = 0; x_col < NUM_CHUNK_DIVISIONS_X; x_col++){
      chunks[z_row][x_col].x_min = world_x_min + (x_col    ) * CHUNK_SIZE;     
      chunks[z_row][x_col].x_max = world_x_min + (x_col + 1) * CHUNK_SIZE;     
      chunks[z_row][x_col].z_min = world_z_min + (z_row    ) * CHUNK_SIZE;     
      chunks[z_row][x_col].z_max = world_z_min + (z_row + 1) * CHUNK_SIZE;     

      chunks[z_row][x_col].z_row_start = (z_row    ) * CHUNK_SIZE;
      chunks[z_row][x_col].z_row_end   = (z_row + 1) * CHUNK_SIZE - 1;
      chunks[z_row][x_col].x_col_start = (x_col    ) * CHUNK_SIZE;     
      chunks[z_row][x_col].x_col_end   = (x_col + 1) * CHUNK_SIZE - 1;     

      // update chunk's y-bounds with max/min voxel height
      chunks[z_row][x_col].y_min = 0.0;
      chunks[z_row][x_col].y_max = MAP_NUM_VOXELS_Y;
    }
  }

  print_log("CHUNK_SIZE: %u", CHUNK_SIZE);
}