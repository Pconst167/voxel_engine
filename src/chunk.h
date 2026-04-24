#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include <stdbool.h>
#include "vector.h"

#define NUM_CHUNK_DIVISIONS 8 /* 10 along x, and 10 along z */

typedef struct{
  uint32_t object_id;
  uint32_t *face_indices;
  uint32_t num_faces;
} chunk_object_faces_t;

typedef struct{
  double x_min, x_max;
  double y_min, y_max;
  double z_min, z_max;
  bool empty;
  /* object data */
  chunk_object_faces_t *chunk_object_faces;
  uint32_t num_chunk_objects;
  /* collision data */
  uint32_t *collision_aabbs; /* list of collision aabb ids */
  uint32_t num_collision_aabbs;
} chunk_t;

/* used to sort chunks for rendering */
typedef struct{
  uint32_t chunk_z_row;
  uint32_t chunk_x_col;
  float sort_z;
} chunk_sort_key_t;

extern chunk_t chunks[NUM_CHUNK_DIVISIONS][NUM_CHUNK_DIVISIONS];
extern double world_x_min;
extern double world_x_max;
extern double world_z_min;
extern double world_z_max;

extern double chunk_dx;
extern double chunk_dz;

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
void create_world_chunks(void);

void populate_chunks_with_objects(void);
void populate_chunks_with_collision_aabbs(void);

int chunk_has_object_entry(int z_row, int x_col, uint32_t object_id);

#endif