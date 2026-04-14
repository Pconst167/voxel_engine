#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "array.h"
#include "mesh.h"
#include "material.h"

mesh_t meshes[MAX_MESHES];
uint32_t num_meshes;

void load_obj_file(char* filename) {
  FILE* file = fopen(filename, "r");
  char line[1024];
  color_t color;
  tex2_t *texture_coords = NULL;

  if (file == NULL) {
    fprintf(stderr, "ERROR: could not open OBJ file: %s\n", filename);
    exit(1);
  }

  while (fgets(line, sizeof(line), file)) {
    //color = ((rand() & 0xFF)) |
    //        ((rand() & 0xFF) << 8) |
    //        ((rand() & 0xFF) << 16) |
    //        (0xFF << 24);

    color.uint32 = 0xFFFFFFFF;

    /* vertex */
    if (strncmp(line, "v ", 2) == 0) {
      vector3_t vertex;
      if (sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3) {
        array_push(meshes[num_meshes].vertices, vertex);
      }
    }
    /* texture */
    else if (strncmp(line, "vt ", 3) == 0) {
      tex2_t texture;
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
        int vertex_count = array_length(meshes[num_meshes].vertices);
        if (vertex0_index < 0) vertex0_index = vertex_count + vertex0_index + 1;
        if (vertex1_index < 0) vertex1_index = vertex_count + vertex1_index + 1;
        if (vertex2_index < 0) vertex2_index = vertex_count + vertex2_index + 1;

        face.vertex_indices[0] = vertex0_index - 1;
        face.vertex_indices[1] = vertex1_index - 1;
        face.vertex_indices[2] = vertex2_index - 1;
        face.color = color;

        if (vt_idx[0] > 0)
          face.uv[0] = texture_coords[vt_idx[0] - 1];

        if (vt_idx[i] > 0)
          face.uv[1] = texture_coords[vt_idx[i] - 1];

        if (vt_idx[i + 1] > 0)
          face.uv[2] = texture_coords[vt_idx[i + 1] - 1];

        array_push(meshes[num_meshes].faces, face);
      }
    }
  }

  num_meshes++;
  array_free(texture_coords);
  fclose(file);
}

mesh_t *load_mesh(
  char *obj_filename, 
  char *png_filename
){
  mesh_t *mesh;
  load_obj_file(obj_filename);

  /* grab pointer to last created mesh */
  mesh = &meshes[num_meshes - 1];

  load_png_texture_data(mesh, png_filename);

  /* compute a bounding box around the mesh */
  int num_vertices = array_length(mesh->vertices);
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

  /* bounding box center*/
  vector3_t bbox_center = {
    0.5f * (bbox_min.x + bbox_max.x),
    0.5f * (bbox_min.y + bbox_max.y),
    0.5f * (bbox_min.z + bbox_max.z)
  };

  /* compute a bounding sphere around the mesh */
  float radius_sq = 0.0f;
  for(int i = 0; i < num_vertices; i++){
    vector3_t d = {
      mesh->vertices[i].x - bbox_center.x,
      mesh->vertices[i].y - bbox_center.y,
      mesh->vertices[i].z - bbox_center.z
    };
    float dist_sq = d.x * d.x + d.y * d.y + d.z * d.z;
    if(dist_sq > radius_sq) radius_sq = dist_sq;
  }

  mesh->bbox_min.x        = bbox_min.x;
  mesh->bbox_min.y        = bbox_min.y;
  mesh->bbox_min.z        = bbox_min.z;
  mesh->bbox_max.x        = bbox_max.x;
  mesh->bbox_max.y        = bbox_max.y;
  mesh->bbox_max.z        = bbox_max.z;
  mesh->bounding_center.x = bbox_center.x;
  mesh->bounding_center.y = bbox_center.y;
  mesh->bounding_center.z = bbox_center.z;
  mesh->bsphere_radius  = sqrtf(radius_sq);

  /* return pointer to mesh */
  return mesh;
}

void load_png_texture_data(mesh_t *mesh, char *filename){
  upng_t *png_texture = upng_new_from_file(filename);
  if(png_texture == NULL){
    fprintf(stderr, "ERROR: could not load PNG file: %s\n", filename);
    exit(1);
  }
  
  upng_decode(png_texture);

  if(upng_get_error(png_texture) != UPNG_EOK){
    fprintf(stderr, "ERROR: could not decode PNG file: %s\n", filename);
    upng_free(png_texture);
    exit(1);
  }

  unsigned width  = upng_get_width(png_texture);
  unsigned height = upng_get_height(png_texture);
  unsigned comps  = upng_get_components(png_texture); // key

  const unsigned char *src = upng_get_buffer(png_texture);

  mesh->texture_width  = width;
  mesh->texture_height = height;

  mesh->mesh_texture = (color_t *)malloc(width * height * sizeof(color_t));
  if(mesh->mesh_texture == NULL){
    fprintf(stderr, "ERROR: could not allocate texture memory\n");
    upng_free(png_texture);
    exit(1);
  }

  // Convert to RGBA always
  for(unsigned i = 0; i < width * height; i++){
    uint8_t r, g, b, a;

    if(comps == 4){
      r = src[i*4 + 0];
      g = src[i*4 + 1];
      b = src[i*4 + 2];
      a = src[i*4 + 3];
    }
    else if(comps == 3){
      r = src[i*3 + 0];
      g = src[i*3 + 1];
      b = src[i*3 + 2];
      a = 0xFF; // force opaque
    }
    else {
      // optional: handle grayscale etc
      r = g = b = src[i];
      a = 0xFF;
    }

    /* ABGR */
    mesh->mesh_texture[i] = (color_t){
      .rgb.alpha = a,
      .rgb.blue  = b,
      .rgb.green = g,
      .rgb.red   = r
    };
  }

  upng_free(png_texture);
}