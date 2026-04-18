#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include <stdbool.h>
#include "object.h"

#define NUM_CHUNK_DIVISIONS 10 /* 10 along x, and 10 along z */

typedef struct{
  uint32_t object_id;
  uint32_t *face_indices;
  uint32_t num_faces;
} chunk_object_faces_t;

typedef struct{
  float x_min, x_max;
  float y_min, y_max;
  float z_min, z_max;
  bool empty;
  chunk_object_faces_t *chunk_object_faces;
  uint32_t num_chunk_objects;
} chunk_t;

extern chunk_t chunks[NUM_CHUNK_DIVISIONS][NUM_CHUNK_DIVISIONS];
extern float world_x_min;
extern float world_x_max;
extern float world_z_min;
extern float world_z_max;

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

/* go through all static/chunkable objects in the world, for each object, get the face vertices
   and keep track of minimum and maximum x, and z.  use those as the world bounds */
void calculate_world_bounds(void);

#endif