#ifndef CHUNK_H
#define CHUNK_H

#include <stdint.h>
#include <stdbool.h>
#include "vector.h"
#include "voxel.h"

typedef enum{
  CHUNK_INTERSECTION_MODE_OUTSIDE,
  CHUNK_INTERSECTION_MODE_INSIDE,
  CHUNK_INTERSECTION_MODE_INTERSECTING
} chunk_intersection_mode_t;

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
  uint32_t z_row_start;
  uint32_t z_row_end;
  uint32_t x_col_start;
  uint32_t x_col_end;
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

extern chunk_t chunks[NUM_CHUNK_DIVISIONS_Z][NUM_CHUNK_DIVISIONS_X];
extern double world_x_min;
extern double world_x_max;
extern double world_z_min;
extern double world_z_max;
extern double world_y_min;
extern double world_y_max;

extern double chunk_dx;
extern double chunk_dz;
extern double chunk_dy;

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