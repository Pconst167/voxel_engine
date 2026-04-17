#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"

#define MAX_MESHES 5000

typedef struct{
  /* geometry */
  vector3_t *vertices; /* dynamic array */
  face_t *faces;       /* dynamic array */
  uint32_t num_vertices;
  uint32_t num_faces;
  /* bounding box/sphere */
  vector3_t bbox_min;
  vector3_t bbox_max;
  vector3_t bounding_center;
  float bsphere_radius;
} mesh_t;

extern mesh_t meshes[MAX_MESHES];
extern uint32_t num_meshes;

mesh_t *load_mesh(
  char *obj_filename
);

void load_obj_file(char* filename);

#endif