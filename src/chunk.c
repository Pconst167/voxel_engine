#include "chunk.h"
#include "array.h"
#include "log.h"
#include "debug.h"

chunk_t chunks[NUM_CHUNK_DIVISIONS_Y][NUM_CHUNK_DIVISIONS_Z][NUM_CHUNK_DIVISIONS_X];

double world_x_min;
double world_x_max;
double world_z_min;
double world_z_max;
double world_y_min;
double world_y_max;

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

  /* set the boundary of each chunk */
  for(uint32_t y_plane = 0; y_plane < NUM_CHUNK_DIVISIONS_Y; y_plane++){
    for(uint32_t z_row = 0; z_row < NUM_CHUNK_DIVISIONS_Z; z_row++){
      for(uint32_t x_col = 0; x_col < NUM_CHUNK_DIVISIONS_X; x_col++){
        chunks[y_plane][z_row][x_col].y_min = world_y_min + (y_plane    ) * CHUNK_SIZE;     
        chunks[y_plane][z_row][x_col].y_max = world_y_min + (y_plane + 1) * CHUNK_SIZE;     
        chunks[y_plane][z_row][x_col].z_min = world_z_min + (z_row      ) * CHUNK_SIZE;     
        chunks[y_plane][z_row][x_col].z_max = world_z_min + (z_row + 1  ) * CHUNK_SIZE;     
        chunks[y_plane][z_row][x_col].x_min = world_x_min + (x_col      ) * CHUNK_SIZE;     
        chunks[y_plane][z_row][x_col].x_max = world_x_min + (x_col + 1  ) * CHUNK_SIZE;     
        
        chunks[y_plane][z_row][x_col].y_plane_start = (y_plane    ) * CHUNK_SIZE;
        chunks[y_plane][z_row][x_col].y_plane_end   = (y_plane + 1) * CHUNK_SIZE - 1;
        chunks[y_plane][z_row][x_col].z_row_start   = (z_row      ) * CHUNK_SIZE;
        chunks[y_plane][z_row][x_col].z_row_end     = (z_row + 1  ) * CHUNK_SIZE - 1;
        chunks[y_plane][z_row][x_col].x_col_start   = (x_col      ) * CHUNK_SIZE;     
        chunks[y_plane][z_row][x_col].x_col_end     = (x_col + 1  ) * CHUNK_SIZE - 1;     
      }
    }
  }

  print_log("CHUNK_SIZE: %u", CHUNK_SIZE);
}