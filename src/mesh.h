#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "triangle.h"
#include "upng.h"

#define MAX_MESHES 5000

typedef struct{
  /* geometry */
  vector3_t *vertices; /* dynamic array */
  face_t *faces;       /* dynamic array */
  /* bounding box/sphere */
  vector3_t bbox_min;
  vector3_t bbox_max;
  vector3_t bounding_center;
  float bsphere_radius;
  /* texture data */
  color_t *mesh_texture;
  uint32_t texture_width;
  uint32_t texture_height;
} mesh_t;

extern mesh_t meshes[MAX_MESHES];
extern uint32_t num_meshes;

mesh_t *load_mesh(
  char *obj_filename, 
  char *png_filename
);

void load_obj_file(char* filename);
void load_png_texture_data(mesh_t *mesh, char* filename);

#endif