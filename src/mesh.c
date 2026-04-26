#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "mesh.h"
#include "color.h"
#include "array.h"
#include "log.h"

mesh_t meshes[MAX_NUM_MESHES];
uint32_t num_meshes;

void load_obj_file(char* filename, mesh_t *mesh) {
  FILE* file = fopen(filename, "r");
  char line[1024];
  uv_t *texture_coords = NULL;

  if(file == NULL) {
    print_log("ERROR: could not open OBJ file: %s\n", filename);
    exit(1);
  }

  while(fgets(line, sizeof(line), file)) {
    /* vertex */
    if (strncmp(line, "v ", 2) == 0) {
      vector3_t vertex;
      if (sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3) {
        array_push(mesh->vertices, vertex);
      }
    }
    /* texture */
    else if (strncmp(line, "vt ", 3) == 0) {
      uv_t texture;
      if (sscanf(line, "vt %f %f", &texture.u, &texture.v) == 2) {
        texture.v = 1.0f - texture.v;
        array_push(texture_coords, texture);
      }
    }
    /* face */
    else if (strncmp(line, "f ", 2) == 0) {
      int v_idx[16];
      int vt_idx[16];
      //int vn_idx[16];
      int num_vertices = 0;

      char *token = strtok(line + 2, " \n");

      while (token != NULL && num_vertices < 16) {
        int v = 0, vt = 0, vn = 0;

        if (sscanf(token, "%d/%d/%d", &v, &vt, &vn) == 3) {
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = vt;
          //vn_idx[num_vertices] = vn;
        }
        else if (sscanf(token, "%d//%d", &v, &vn) == 2) {
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = 0;
          //vn_idx[num_vertices] = vn;
        }
        else if (sscanf(token, "%d/%d", &v, &vt) == 2) {
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = vt;
          //vn_idx[num_vertices] = 0;
        }
        else if (sscanf(token, "%d", &v) == 1) {
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = 0;
          //vn_idx[num_vertices] = 0;
        }

        num_vertices++;
        token = strtok(NULL, " \n");
      }

      /* triangulate polygon using fan */
      for (int i = 1; i < num_vertices - 1; i++) {
        face_t face;

        int vertex0_index = v_idx[0];
        int vertex1_index = v_idx[i];
        int vertex2_index = v_idx[i + 1];

        /* handle negative indices */
        int vertex_count = array_length(mesh->vertices);
        if (vertex0_index < 0) vertex0_index = vertex_count + vertex0_index + 1;
        if (vertex1_index < 0) vertex1_index = vertex_count + vertex1_index + 1;
        if (vertex2_index < 0) vertex2_index = vertex_count + vertex2_index + 1;

        face.vertex_indices[0] = vertex0_index - 1;
        face.vertex_indices[1] = vertex1_index - 1;
        face.vertex_indices[2] = vertex2_index - 1;

        if (vt_idx[0] > 0)
          face.uv[0] = texture_coords[vt_idx[0] - 1];

        if (vt_idx[i] > 0)
          face.uv[1] = texture_coords[vt_idx[i] - 1];

        if (vt_idx[i + 1] > 0)
          face.uv[2] = texture_coords[vt_idx[i + 1] - 1];

        array_push(mesh->faces, face);
      }
    }
  }

  mesh->num_vertices = array_length(mesh->vertices);
  mesh->num_faces    = array_length(mesh->faces);

  if(mesh->num_vertices == 0){
    print_log("ERROR: 0 vertices in OBJ file: %s\n", filename);
    exit(1);
  }

  if(mesh->num_faces == 0){
    print_log("ERROR: 0 faces in OBJ file: %s\n", filename);
    exit(1);
  }

  array_free(texture_coords);
  fclose(file);
}

mesh_t *load_mesh(char *obj_filename){
  if(num_meshes >= MAX_NUM_MESHES){
    print_log("ERROR: max number of meshes reached %d", MAX_NUM_MESHES);
    return NULL;
  }

  mesh_t *mesh; // pointer to last mesh slot (the new mesh)

  /* grab pointer to last mesh slot */
  mesh = &meshes[num_meshes];

  load_obj_file(obj_filename, mesh); // load obj file into new mesh

  /* compute a bounding box around the mesh */
  int num_vertices = mesh->num_vertices;

  vector3_t bbox_min = mesh->vertices[0];
  vector3_t bbox_max = mesh->vertices[0];
  for(int i = 0; i < num_vertices; i++){
    if(mesh->vertices[i].x < bbox_min.x) bbox_min.x = mesh->vertices[i].x;
    if(mesh->vertices[i].y < bbox_min.y) bbox_min.y = mesh->vertices[i].y;
    if(mesh->vertices[i].z < bbox_min.z) bbox_min.z = mesh->vertices[i].z;

    if(mesh->vertices[i].x > bbox_max.x) bbox_max.x = mesh->vertices[i].x;
    if(mesh->vertices[i].y > bbox_max.y) bbox_max.y = mesh->vertices[i].y;
    if(mesh->vertices[i].z > bbox_max.z) bbox_max.z = mesh->vertices[i].z;
  }

  /* bounding box center */
  double bbox_center_x = 0.5 * ((double)bbox_min.x + (double)bbox_max.x);
  double bbox_center_y = 0.5 * ((double)bbox_min.y + (double)bbox_max.y);
  double bbox_center_z = 0.5 * ((double)bbox_min.z + (double)bbox_max.z);

  /* compute a bounding sphere around the mesh */
  double radius_sq = 0.0f;
  for(int i = 0; i < num_vertices; i++){
    double dx = (double)mesh->vertices[i].x - bbox_center_x;
    double dy = (double)mesh->vertices[i].y - bbox_center_y;
    double dz = (double)mesh->vertices[i].z - bbox_center_z;
    
    double dist_sq = dx*dx + dy*dy + dz*dz;
    if(dist_sq > radius_sq) radius_sq = dist_sq;
  }

  mesh->bbox_aabb.x_min   = bbox_min.x;
  mesh->bbox_aabb.x_max   = bbox_max.x;
  mesh->bbox_aabb.y_min   = bbox_min.y;
  mesh->bbox_aabb.y_max   = bbox_max.y;
  mesh->bbox_aabb.z_min   = bbox_min.z;
  mesh->bbox_aabb.z_max   = bbox_max.z;

  mesh->bounding_center.x = (float)bbox_center_x;
  mesh->bounding_center.y = (float)bbox_center_y;
  mesh->bounding_center.z = (float)bbox_center_z;
  mesh->bsphere_radius    = (float)sqrt(radius_sq);

  num_meshes++;

  /* return pointer to mesh */
  return mesh;
}

mesh_t *create_cube_mesh(vector3_t vertex_min, vector3_t vertex_max){
  if(num_meshes >= MAX_NUM_MESHES){
    print_log("ERROR: max number of meshes reached %d", MAX_NUM_MESHES);
    return NULL;
  }

  mesh_t *mesh = &meshes[num_meshes]; // pointer to last mesh slot (the new mesh)
  mesh->num_vertices = 8;
  mesh->num_faces    = 12;
  vector3_t vertices[8];

  for(uint8_t v = 0; v < 8; v++){
    vertices[v] = (vector3_t){
      .x = (v & 0x04) ? vertex_max.x : vertex_min.x,
      .y = (v & 0x02) ? vertex_max.y : vertex_min.y,
      .z = (v & 0x01) ? vertex_max.z : vertex_min.z
    };
    array_push(mesh->vertices, vertices[v]);
  }

  // clockwise winding viewed from outside
  face_t faces[12] = {
    {.vertex_indices = {0, 2, 6}, .uv = {{0,1}, {0,0}, {1,0}}}, // -z
    {.vertex_indices = {0, 6, 4}, .uv = {{0,1}, {1,0}, {1,1}}},

    {.vertex_indices = {1, 7, 3}, .uv = {{0,1}, {1,0}, {0,0}}}, // +z
    {.vertex_indices = {1, 5, 7}, .uv = {{0,1}, {1,1}, {1,0}}},

    {.vertex_indices = {0, 1, 3}, .uv = {{0,1}, {0,0}, {1,0}}}, // -x
    {.vertex_indices = {0, 3, 2}, .uv = {{0,1}, {1,0}, {1,1}}},

    {.vertex_indices = {4, 7, 5}, .uv = {{0,1}, {1,0}, {0,0}}}, // +x
    {.vertex_indices = {4, 6, 7}, .uv = {{0,1}, {1,1}, {1,0}}},

    {.vertex_indices = {0, 5, 1}, .uv = {{0,1}, {1,0}, {0,0}}}, // -y
    {.vertex_indices = {0, 4, 5}, .uv = {{0,1}, {1,1}, {1,0}}},

    {.vertex_indices = {2, 3, 7}, .uv = {{0,1}, {0,0}, {1,0}}}, // +y
    {.vertex_indices = {2, 7, 6}, .uv = {{0,1}, {1,0}, {1,1}}}
  };

  for(uint8_t i = 0; i < 12; i++)
    array_push(mesh->faces, faces[i]);

  /* bounding box center */
  double bbox_center_x = 0.5 * ((double)vertex_min.x + (double)vertex_max.x);
  double bbox_center_y = 0.5 * ((double)vertex_min.y + (double)vertex_max.y);
  double bbox_center_z = 0.5 * ((double)vertex_min.z + (double)vertex_max.z);

  /* compute a bounding sphere around the mesh */
  double dx     = (double)vertex_max.x - bbox_center_x;
  double dy     = (double)vertex_max.y - bbox_center_y;
  double dz     = (double)vertex_max.z - bbox_center_z;
  double radius = sqrt(dx*dx + dy*dy + dz*dz);

  mesh->bbox_aabb.x_min   = vertex_min.x;
  mesh->bbox_aabb.x_max   = vertex_max.x;
  mesh->bbox_aabb.y_min   = vertex_min.y;
  mesh->bbox_aabb.y_max   = vertex_max.y;
  mesh->bbox_aabb.z_min   = vertex_min.z;
  mesh->bbox_aabb.z_max   = vertex_max.z;

  mesh->bounding_center.x = (float)bbox_center_x;
  mesh->bounding_center.y = (float)bbox_center_y;
  mesh->bounding_center.z = (float)bbox_center_z;
  mesh->bsphere_radius    = (float)radius;

  num_meshes++;

  // return pointer to mesh
  return mesh;
}