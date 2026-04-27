#ifndef MESH_H
#define MESH_H

#include "vector.h"
#include "material.h"
#include "aabb.h"

#define MAX_NUM_MESHES 1000

typedef struct{
  float u, v;
} uv_t;

typedef struct{
  uint32_t vertex_indices[3];
  uv_t uv[3];
  color_t color;
  uint32_t material_id; // each face has its own material - keeps material id
} face_t;

typedef struct{
  /* geometry */
  vector3_t *vertices; /* dynamic array */
  face_t *faces;       /* dynamic array */
  uint32_t num_vertices;
  uint32_t num_faces;
  /* bounding box/sphere */
  aabb_3d_t bbox_aabb;
  vector3_t bounding_center;
  float bsphere_radius;
  // materials
  material_t *materials;
  uint32_t num_materials;
} mesh_t;

extern mesh_t meshes[MAX_NUM_MESHES];
extern uint32_t num_meshes;

mesh_t *load_mesh(char *obj_filename);

void load_obj_file(char* filename, mesh_t *mesh);

mesh_t *create_cube_mesh(vector3_t vertex_min, vector3_t vertex_max);

#endif