#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "mesh.h"
#include "color.h"
#include "array.h"
#include "log.h"
#include "texture.h"

mesh_t meshes[MAX_NUM_MESHES];
uint32_t num_meshes;

// used only internally - does not print log failures because it's handled by other functions
static int find_mesh_material_id(mesh_t *mesh, const char *material_name){
  int mesh_num_materials = mesh->num_materials;

  for(int material_i = 0; material_i < mesh_num_materials; material_i++)
    if(!strcmp(mesh->materials[material_i].material_name, material_name)) 
      return material_i;

  return -1;
}

static uint32_t add_material_to_mesh(mesh_t *mesh, material_t material){
  int material_id = find_mesh_material_id(mesh, material.material_name);

  // if material exists, return index
  if(material_id != -1) return (uint32_t)material_id;

  // if material doesn't exist, add material to mesh
  array_push(mesh->materials, material);
  mesh->num_materials++;

  return mesh->num_materials - 1;
}

static void get_directory_from_path(
  const char *path, 
  char *output_directory, 
  size_t output_size
){
  const char *last_slash = strrchr(path, '/'); // strrchr finds first occurrence starting at the end

  // if no slash is found at all, then file has no directory, i.e 'file.obj'
  // hence return '.' (current directory)
  if(!last_slash){
    snprintf(output_directory, output_size, ".");
    return;
  }

  // returned path is something like './some/directory/' or '/some/other/'
  // and output_directory needs to include a '\0' at the end, so need to test if len >= output_size
  size_t len = (size_t)(last_slash - path);
  if(len >= output_size){
    output_directory[0] = '\0';
    print_log("[ERROR] path too long\n");
    return;
  }

  // copy final directory into output_directory and end with '\0'
  memcpy(output_directory, path, len);
  output_directory[len] = '\0';
}

// join two paths together, i.e: './some/dir' or '/some/dir/' and 'filename.mtl' = './some/dir/filename.mtl' or '/some/dir/filename.mtl'
static void path_join(
  char *output, 
  size_t output_size, 
  const char *directory, 
  const char *filename
){
  // if filenamename starts with '/', then it already has a full path with directory so return filename
  if(filename[0] == '/')
    snprintf(output, output_size, "%s", filename);
  // if filename does not contain a root slash '/', then join directory with filename
  else
    snprintf(output, output_size, "%s/%s", directory, filename);
}

// load material file referenced by .obj file
// .mtl file location is relative to .obj directory
static void load_mtl_file(
  const char *mtl_filename, 
  const char *obj_directory, 
  mesh_t *mesh
){
  // join .obj directory with .mtl filename to form a full path
  char full_path[2048];
  path_join(full_path, sizeof(full_path), obj_directory, mtl_filename);

  FILE *file = fopen(full_path, "r");
  if(!file){
    print_log("WARNING: could not open .mtl file: %s\n", full_path);
    return;
  }

  print_log("loading material file: %s", mtl_filename);

  char line[1024];
  // generate a default material, in case material file doesn't define any materials in error
  material_t current_material = generate_default_material("default");
  // flag to say whether we have found a new material name definition
  bool has_current_material = false;

  // start reading file lines
  while(fgets(line, sizeof(line), file)){
    if(strncmp(line, "newmtl ", 7) == 0){ // if we find 'newmtl' keyword, then if we have already parsed a previous material, commit it 
      if(has_current_material){
        add_material_to_mesh(mesh, current_material);
      }

      char material_name[128];
      // if 'newmtl' keyword is found and it also has the new material name, then generate new default material
      // and set the flag to true, indicating we have found a new material definition + name
      if(sscanf(line, "newmtl %127[^\r\n]", material_name) == 1){ 
        current_material = generate_default_material(material_name);
        has_current_material = true;
        print_log("new material name: %s", material_name);
      }
    }
    // base color
    else if(has_current_material && strncmp(line, "Kd ", 3) == 0){
      float r, g, b;
      if(sscanf(line, "Kd %f %f %f", &r, &g, &b) == 3)
        current_material.base_color = color_from_float_rgb(r, g, b);
    }
    // emissive color
    else if(has_current_material && strncmp(line, "Ke ", 3) == 0){
      float r, g, b;
      if(sscanf(line, "Ke %f %f %f", &r, &g, &b) == 3)
        current_material.emissive_color = color_from_float_rgb(r, g, b);
    }
    // opacity
    else if(has_current_material && strncmp(line, "d ", 2) == 0){
      float opacity;
      if(sscanf(line, "d %f", &opacity) == 1){
        current_material.opacity = opacity;

        // if opacity is less than 1.0, then material is no longer opaque and is alpha blended
        if(opacity < 1.0f) current_material.blend_mode = BLEND_MODE_ALPHA_BLEND;
      }
    }
    // reflects light / specular highlights
    else if(has_current_material && strncmp(line, "illum ", 6) == 0){
      int illum;
      if(sscanf(line, "illum %d", &illum) == 1){
        switch(illum){
          case 0:
            current_material.reflects_light      = false;
            current_material.specular_highlights = false;
            break;
          case 1:
            current_material.reflects_light      = true;
            current_material.specular_highlights = false;
            break;
          default:
            current_material.reflects_light      = true;
            current_material.specular_highlights = true;
            break;
        }
      }
    }
    // texture
    else if(has_current_material && strncmp(line, "map_Kd ", 7) == 0){
      char texture_filename[512];
      if(sscanf(line, "map_Kd %511[^\r\n]", texture_filename) == 1)
        current_material.texture = load_texture(texture_filename);
    }
  }

  // if file end is reached, commit current material
  if(has_current_material){
    add_material_to_mesh(mesh, current_material);
  }

  fclose(file);
}

void load_obj_file(char *filename, mesh_t *mesh, bool invert_x){
  FILE *file = fopen(filename, "r");
  char line[1024];
  uv_t *texture_coords = NULL;

  uint32_t current_material_id = 0;

  char obj_dir[1024];
  get_directory_from_path(filename, obj_dir, sizeof(obj_dir));

  if(file == NULL){
    print_log("[ERROR] could not open OBJ file: %s\n", filename);
    exit(1);
  }

  print_log("loading obj file: %s", filename);

  // default material in case .obj has no .mtl/usemtl
  add_material_to_mesh(mesh, generate_default_material("default"));
  current_material_id = 0;

  while(fgets(line, sizeof(line), file)){
    // material library
    if(strncmp(line, "mtllib ", 7) == 0){
      char mtl_filename[512];

      // read material filename
      if(sscanf(line, "mtllib %511[^\r\n]", mtl_filename) == 1)
        load_mtl_file(mtl_filename, obj_dir, mesh);
    }
    // use material
    else if(strncmp(line, "usemtl ", 7) == 0){
      char material_name[128];

      if(sscanf(line, "usemtl %127[^\r\n]", material_name) == 1){
        int material_id = find_mesh_material_id(mesh, material_name);

        // if material name found, set current id to it
        if(material_id != -1)
          current_material_id = (uint32_t)material_id;
        else{
          // if material not found, create new material slot, and fill it with default material
          material_t missing_material = generate_default_material(material_name);
          current_material_id = add_material_to_mesh(mesh, missing_material);

          print_log("WARNING: material '%s' does not exist or material library not set in .obj file: %s - generating default material slot", material_name, filename);
        }
      }
    }
    /* vertex */
    else if(strncmp(line, "v ", 2) == 0){
      vector3_t vertex;
      if(sscanf(line, "v %f %f %f", &vertex.x, &vertex.y, &vertex.z) == 3){
        if(invert_x) vertex.x = -vertex.x;
        array_push(mesh->vertices, vertex);
      }
    }
    /* texture coord */
    else if(strncmp(line, "vt ", 3) == 0){
      uv_t texture;
      if(sscanf(line, "vt %f %f", &texture.u, &texture.v) == 2){
        texture.v = 1.0f - texture.v;
        array_push(texture_coords, texture);
      }
    }
    /* face */
    else if(strncmp(line, "f ", 2) == 0){
      int v_idx[16];
      int vt_idx[16];
      int num_vertices = 0;

      char *token = strtok(line + 2, " \n\r");

      while(token != NULL && num_vertices < 16){
        int v = 0, vt = 0, vn = 0;
        bool parsed_vertex_ok = false;

        if(sscanf(token, "%d/%d/%d", &v, &vt, &vn) == 3){
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = vt;
          parsed_vertex_ok = true;
        }
        else if(sscanf(token, "%d//%d", &v, &vn) == 2){
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = 0;
          parsed_vertex_ok = true;
        }
        else if(sscanf(token, "%d/%d", &v, &vt) == 2){
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = vt;
          parsed_vertex_ok = true;
        }
        else if(sscanf(token, "%d", &v) == 1){
          v_idx[num_vertices]  = v;
          vt_idx[num_vertices] = 0;
          parsed_vertex_ok = true;
        }

        if(parsed_vertex_ok) num_vertices++;
        token = strtok(NULL, " \n\r");
      }

      /* triangulate polygon using fan */
      for(int i = 1; i < num_vertices - 1; i++){
        face_t face;
        memset(&face, 0, sizeof(face));

        face.material_id = current_material_id;

        int vertex0_index = v_idx[0];
        int vertex1_index = v_idx[i];
        int vertex2_index = v_idx[i + 1];

        int texture0_index = vt_idx[0];
        int texture1_index = vt_idx[i];
        int texture2_index = vt_idx[i + 1];

        int vertex_count  = array_length(mesh->vertices);
        int texture_count = array_length(texture_coords);

        // handle negative vertex indices
        if(vertex0_index < 0) vertex0_index = vertex_count + vertex0_index + 1;
        if(vertex1_index < 0) vertex1_index = vertex_count + vertex1_index + 1;
        if(vertex2_index < 0) vertex2_index = vertex_count + vertex2_index + 1;

        // handle negative texture indices
        if(texture0_index < 0) texture0_index = texture_count + texture0_index + 1;
        if(texture1_index < 0) texture1_index = texture_count + texture1_index + 1;
        if(texture2_index < 0) texture2_index = texture_count + texture2_index + 1;

        face.vertex_indices[0] = vertex0_index - 1;
        face.vertex_indices[1] = vertex1_index - 1;
        face.vertex_indices[2] = vertex2_index - 1;

        if(texture0_index > 0) face.uv[0] = texture_coords[texture0_index - 1];
        if(texture1_index > 0) face.uv[1] = texture_coords[texture1_index - 1];
        if(texture2_index > 0) face.uv[2] = texture_coords[texture2_index - 1];

        if(invert_x){
          uint32_t temp_vertex;
          uv_t temp_uv;
          temp_vertex = face.vertex_indices[1];
          temp_uv = face.uv[1];
          face.vertex_indices[1] = face.vertex_indices[2];
          face.vertex_indices[2] = temp_vertex;
          face.uv[1] = face.uv[2];
          face.uv[2] = temp_uv;
        }

        array_push(mesh->faces, face);
      }
    }
  }

  mesh->num_vertices  = array_length(mesh->vertices);
  mesh->num_faces     = array_length(mesh->faces);
  mesh->num_materials = array_length(mesh->materials);

  if(mesh->num_vertices == 0){
    print_log("[ERROR] 0 vertices in OBJ file: %s\n", filename);
    exit(1);
  }

  if(mesh->num_faces == 0){
    print_log("[ERROR] 0 faces in OBJ file: %s\n", filename);
    exit(1);
  }

  array_free(texture_coords);
  fclose(file);
}

mesh_t *load_mesh(char *obj_filename, bool invert_x){
  if(num_meshes >= MAX_NUM_MESHES){
    print_log("[ERROR] max number of meshes reached %d", MAX_NUM_MESHES);
    return NULL;
  }

  mesh_t *mesh; // pointer to last mesh slot (the new mesh)

  /* grab pointer to last mesh slot */
  mesh = &meshes[num_meshes];

  load_obj_file(obj_filename, mesh, invert_x); // load obj file into new mesh

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
    print_log("[ERROR] max number of meshes reached %d", MAX_NUM_MESHES);
    return NULL;
  }

  mesh_t *mesh = &meshes[num_meshes]; // pointer to last mesh slot (the new mesh)
  mesh->num_vertices = 8;
  mesh->num_faces    = 12;

  // set default material
  add_material_to_mesh(mesh, generate_default_material("default"));

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

  for(uint8_t i = 0; i < 12; i++){
    array_push(mesh->faces, faces[i]);
    mesh->faces[i].material_id = 0; // set default material for all faces
  }

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

// used by other modules to search for mesh material by name
material_t *get_mesh_material_by_name(mesh_t *mesh, const char *material_name){
  uint32_t mesh_num_materials = mesh->num_materials;

  for(uint32_t material_i = 0; material_i < mesh_num_materials; material_i++)
    if(!strcmp(mesh->materials[material_i].material_name, material_name)) 
      return &mesh->materials[material_i];

  print_log("[ERROR] mesh material '%s' not found by name", material_name);

  return NULL;
}
static float terrain_hash_noise(int x, int z, uint32_t seed){
  uint32_t hash = (uint32_t)x * 374761393u + (uint32_t)z * 668265263u + seed * 2246822519u;
  hash = (hash ^ (hash >> 13u)) * 1274126177u;
  hash = hash ^ (hash >> 16u);

  return (float)(hash & 0x00FFFFFFu) / (float)0x00FFFFFFu;
}

static float terrain_smoothstep(float value){
  return value * value * (3.0f - 2.0f * value);
}

static float terrain_lerp(float a, float b, float t){
  return a + (b - a) * t;
}

static float terrain_value_noise(float x, float z, uint32_t seed){
  int x0 = (int)floorf(x);
  int z0 = (int)floorf(z);
  int x1 = x0 + 1;
  int z1 = z0 + 1;

  float tx = terrain_smoothstep(x - (float)x0);
  float tz = terrain_smoothstep(z - (float)z0);

  float h00 = terrain_hash_noise(x0, z0, seed);
  float h10 = terrain_hash_noise(x1, z0, seed);
  float h01 = terrain_hash_noise(x0, z1, seed);
  float h11 = terrain_hash_noise(x1, z1, seed);

  float hx0 = terrain_lerp(h00, h10, tx);
  float hx1 = terrain_lerp(h01, h11, tx);

  return terrain_lerp(hx0, hx1, tz);
}

static float terrain_height(float x, float z, uint32_t seed){
  float height = 0.0f;
  float amplitude = 1.0f;
  float frequency = 0.045f;
  float amplitude_sum = 0.0f;

  for(int octave = 0; octave < 5; octave++){
    height += terrain_value_noise(x * frequency, z * frequency, seed + octave * 9176u) * amplitude;
    amplitude_sum += amplitude;

    amplitude *= 0.5f;
    frequency *= 2.0f;
  }

  height /= amplitude_sum;

  return height;
}

static void terrain_update_bbox(
  vector3_t vertex,
  vector3_t *bbox_min,
  vector3_t *bbox_max,
  bool *bbox_valid
){
  if(!(*bbox_valid)){
    *bbox_min = vertex;
    *bbox_max = vertex;
    *bbox_valid = true;
    return;
  }

  if(vertex.x < bbox_min->x) bbox_min->x = vertex.x;
  if(vertex.y < bbox_min->y) bbox_min->y = vertex.y;
  if(vertex.z < bbox_min->z) bbox_min->z = vertex.z;

  if(vertex.x > bbox_max->x) bbox_max->x = vertex.x;
  if(vertex.y > bbox_max->y) bbox_max->y = vertex.y;
  if(vertex.z > bbox_max->z) bbox_max->z = vertex.z;
}

static void terrain_add_top_square(
  mesh_t *mesh,
  float x0,
  float x1,
  float z0,
  float z1,
  float world_y,
  vector3_t *bbox_min,
  vector3_t *bbox_max,
  bool *bbox_valid
){
  uint32_t base_vertex_index = array_length(mesh->vertices);

  vector3_t vertex0 = {.x = x0, .y = world_y, .z = z0};
  vector3_t vertex1 = {.x = x0, .y = world_y, .z = z1};
  vector3_t vertex2 = {.x = x1, .y = world_y, .z = z1};
  vector3_t vertex3 = {.x = x1, .y = world_y, .z = z0};

  array_push(mesh->vertices, vertex0);
  array_push(mesh->vertices, vertex1);
  array_push(mesh->vertices, vertex2);
  array_push(mesh->vertices, vertex3);

  terrain_update_bbox(vertex0, bbox_min, bbox_max, bbox_valid);
  terrain_update_bbox(vertex1, bbox_min, bbox_max, bbox_valid);
  terrain_update_bbox(vertex2, bbox_min, bbox_max, bbox_valid);
  terrain_update_bbox(vertex3, bbox_min, bbox_max, bbox_valid);

  face_t face0;
  memset(&face0, 0, sizeof(face0));
  face0.material_id = 0;
  face0.vertex_indices[0] = base_vertex_index + 0;
  face0.vertex_indices[1] = base_vertex_index + 1;
  face0.vertex_indices[2] = base_vertex_index + 2;
  face0.uv[0] = (uv_t){0.0f, 0.0f};
  face0.uv[1] = (uv_t){0.0f, 1.0f};
  face0.uv[2] = (uv_t){1.0f, 1.0f};
  array_push(mesh->faces, face0);

  face_t face1;
  memset(&face1, 0, sizeof(face1));
  face1.material_id = 0;
  face1.vertex_indices[0] = base_vertex_index + 0;
  face1.vertex_indices[1] = base_vertex_index + 2;
  face1.vertex_indices[2] = base_vertex_index + 3;
  face1.uv[0] = (uv_t){0.0f, 0.0f};
  face1.uv[1] = (uv_t){1.0f, 1.0f};
  face1.uv[2] = (uv_t){1.0f, 0.0f};
  array_push(mesh->faces, face1);
}

static void terrain_add_vertical_wall(
  mesh_t *mesh,
  vector3_t bottom0,
  vector3_t bottom1,
  vector3_t top1,
  vector3_t top0,
  vector3_t *bbox_min,
  vector3_t *bbox_max,
  bool *bbox_valid
){
  uint32_t base_vertex_index = array_length(mesh->vertices);

  array_push(mesh->vertices, bottom0);
  array_push(mesh->vertices, bottom1);
  array_push(mesh->vertices, top1);
  array_push(mesh->vertices, top0);

  terrain_update_bbox(bottom0, bbox_min, bbox_max, bbox_valid);
  terrain_update_bbox(bottom1, bbox_min, bbox_max, bbox_valid);
  terrain_update_bbox(top1, bbox_min, bbox_max, bbox_valid);
  terrain_update_bbox(top0, bbox_min, bbox_max, bbox_valid);

  face_t face0;
  memset(&face0, 0, sizeof(face0));
  face0.material_id = 1;
  face0.vertex_indices[0] = base_vertex_index + 0;
  face0.vertex_indices[1] = base_vertex_index + 2;
  face0.vertex_indices[2] = base_vertex_index + 1;
  face0.uv[0] = (uv_t){0.0f, 1.0f};
  face0.uv[1] = (uv_t){1.0f, 0.0f};
  face0.uv[2] = (uv_t){1.0f, 1.0f};
  array_push(mesh->faces, face0);

  face_t face1;
  memset(&face1, 0, sizeof(face1));
  face1.material_id = 1;
  face1.vertex_indices[0] = base_vertex_index + 0;
  face1.vertex_indices[1] = base_vertex_index + 3;
  face1.vertex_indices[2] = base_vertex_index + 2;
  face1.uv[0] = (uv_t){0.0f, 1.0f};
  face1.uv[1] = (uv_t){0.0f, 0.0f};
  face1.uv[2] = (uv_t){1.0f, 0.0f};
  array_push(mesh->faces, face1);
}

mesh_t *create_terrain_mesh(void){
  if(num_meshes >= MAX_NUM_MESHES){
    print_log("[ERROR] max number of meshes reached %d", MAX_NUM_MESHES);
    return NULL;
  }

  const int terrain_squares_x = 200;
  const int terrain_squares_z = 200;
  const float square_size = 10.0f;
  const float maximum_height = 80.0f;
  const float height_step = 5.0f;
  const uint32_t seed = 12345u;

  float terrain_heights[200][200];

  mesh_t *mesh = &meshes[num_meshes];

  add_material_to_mesh(mesh, generate_default_material("top"));
  add_material_to_mesh(mesh, generate_default_material("sides"));

  float terrain_width = (float)terrain_squares_x * square_size;
  float terrain_depth = (float)terrain_squares_z * square_size;

  float x_start = -terrain_width * 0.5f;
  float z_start = -terrain_depth * 0.5f;

  vector3_t bbox_min = {0};
  vector3_t bbox_max = {0};
  bool bbox_valid = false;

  for(int z = 0; z < terrain_squares_z; z++){
    for(int x = 0; x < terrain_squares_x; x++){
      float height01 = terrain_height((float)x, (float)z, seed);
      terrain_heights[z][x] = roundf((height01 * maximum_height) / height_step) * height_step;
    }
  }

  for(int z = 0; z < terrain_squares_z; z++){
    for(int x = 0; x < terrain_squares_x; x++){
      float world_y = terrain_heights[z][x];

      float x0 = x_start + (float)x * square_size;
      float x1 = x0 + square_size;
      float z0 = z_start + (float)z * square_size;
      float z1 = z0 + square_size;

      terrain_add_top_square(
        mesh,
        x0,
        x1,
        z0,
        z1,
        world_y,
        &bbox_min,
        &bbox_max,
        &bbox_valid
      );

      float left_height = 0.0f;
      if(x > 0) left_height = terrain_heights[z][x - 1];

      if(world_y > left_height){
        terrain_add_vertical_wall(
          mesh,
          (vector3_t){x0, left_height, z1},
          (vector3_t){x0, left_height, z0},
          (vector3_t){x0, world_y, z0},
          (vector3_t){x0, world_y, z1},
          &bbox_min,
          &bbox_max,
          &bbox_valid
        );
      }

      float right_height = 0.0f;
      if(x < terrain_squares_x - 1) right_height = terrain_heights[z][x + 1];

      if(world_y > right_height){
        terrain_add_vertical_wall(
          mesh,
          (vector3_t){x1, right_height, z0},
          (vector3_t){x1, right_height, z1},
          (vector3_t){x1, world_y, z1},
          (vector3_t){x1, world_y, z0},
          &bbox_min,
          &bbox_max,
          &bbox_valid
        );
      }

      float back_height = 0.0f;
      if(z > 0) back_height = terrain_heights[z - 1][x];

      if(world_y > back_height){
        terrain_add_vertical_wall(
          mesh,
          (vector3_t){x0, back_height, z0},
          (vector3_t){x1, back_height, z0},
          (vector3_t){x1, world_y, z0},
          (vector3_t){x0, world_y, z0},
          &bbox_min,
          &bbox_max,
          &bbox_valid
        );
      }

      float front_height = 0.0f;
      if(z < terrain_squares_z - 1) front_height = terrain_heights[z + 1][x];

      if(world_y > front_height){
        terrain_add_vertical_wall(
          mesh,
          (vector3_t){x1, front_height, z1},
          (vector3_t){x0, front_height, z1},
          (vector3_t){x0, world_y, z1},
          (vector3_t){x1, world_y, z1},
          &bbox_min,
          &bbox_max,
          &bbox_valid
        );
      }
    }
  }

  mesh->num_vertices = array_length(mesh->vertices);
  mesh->num_faces = array_length(mesh->faces);
  mesh->num_materials = array_length(mesh->materials);

  double bbox_center_x = 0.5 * ((double)bbox_min.x + (double)bbox_max.x);
  double bbox_center_y = 0.5 * ((double)bbox_min.y + (double)bbox_max.y);
  double bbox_center_z = 0.5 * ((double)bbox_min.z + (double)bbox_max.z);

  double radius_sq = 0.0;
  for(int i = 0; i < mesh->num_vertices; i++){
    double dx = (double)mesh->vertices[i].x - bbox_center_x;
    double dy = (double)mesh->vertices[i].y - bbox_center_y;
    double dz = (double)mesh->vertices[i].z - bbox_center_z;

    double dist_sq = dx*dx + dy*dy + dz*dz;
    if(dist_sq > radius_sq) radius_sq = dist_sq;
  }

  mesh->bbox_aabb.x_min = bbox_min.x;
  mesh->bbox_aabb.x_max = bbox_max.x;
  mesh->bbox_aabb.y_min = bbox_min.y;
  mesh->bbox_aabb.y_max = bbox_max.y;
  mesh->bbox_aabb.z_min = bbox_min.z;
  mesh->bbox_aabb.z_max = bbox_max.z;

  mesh->bounding_center.x = (float)bbox_center_x;
  mesh->bounding_center.y = (float)bbox_center_y;
  mesh->bounding_center.z = (float)bbox_center_z;
  mesh->bsphere_radius = (float)sqrt(radius_sq);

  num_meshes++;

  return mesh;
}