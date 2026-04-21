#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include <stdbool.h>
#include "object.h"

#define NUM_CHUNK_DIVISIONS 16 /* 10 along x, and 10 along z */

typedef struct{
  double x_min, y_min, z_min;
  double x_max, y_max, z_max;
} aabb_3d_t;

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
  chunk_object_faces_t *chunk_object_faces;
  uint32_t num_chunk_objects;
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
void create_chunks(void);
void populate_chunks(void);
int chunk_has_object_entry(int z_row, int x_col, uint32_t object_id);

aabb_3d_t calculate_3d_aabb(
  vector3_t v_a,
  vector3_t v_b,
  vector3_t v_c
);

#endif