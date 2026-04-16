#include "object.h"
#include "array.h"
#include "matrix.h"

#define NUM_CHUNK_DIVISIONS 10 /* 10 along x, and 10 along z */

typedef struct{
  uint32_t object_id;
  uint32_t *faces;
} chunk_object_faces_t;

typedef struct{
  float x_min, x_max;
  float z_min, z_max;
  chunk_object_faces_t *object_faces;
} chunk_t;

chunk_t chunks[NUM_CHUNK_DIVISIONS][NUM_CHUNK_DIVISIONS];

object_t objects[MAX_NUM_OBJECTS];
uint32_t num_objects;

object_t *create_object(
  mesh_t *mesh,
  object_type_t object_type,
  bool fixed,
  bool backface_culling,

  color_t solid_color,
  color_t emissive_color,
  float emissive_intensity,
  float opacity,
  bool two_sided_lighting,
  bool reflective,
  bool specular,

  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,

  vector3_t scale,
  vector3_t rotation,
  vector3_t translation,
  vector3_t velocity,
  vector3_t angular_velocity,

  texture_t *texture
){
  object_t *object = &objects[num_objects];

  object->mesh               = mesh;
  object->object_type        = object_type;
  object->fixed              = fixed;
  object->backface_culling   = backface_culling;
  
  object->solid_color        = solid_color;
  object->emissive_color     = emissive_color;
  object->emissive_intensity = emissive_intensity;
  object->opacity            = opacity;
  object->two_sided_lighting = two_sided_lighting;
  object->reflective         = reflective;
  object->specular           = specular;
  
  object->blend_mode         = blend_mode;
  object->alpha_cutoff       = alpha_cutoff;
  
  object->scale              = scale;
  object->rotation           = rotation;
  object->translation        = translation;
  object->velocity           = velocity;
  object->angular_velocity   = angular_velocity;
  
  object->texture            = texture;

  /* if object is fixed/static, we bake in all the transformations beforehand! */
  if(fixed){
    matrix4_t world_matrix = matrix4_gen_world_matrix(object);

    for(uint32_t i = 0; i < mesh->num_vertices; i++){
      vector4_t transformed_vertex = matrix4_mul_vector3(&world_matrix, (vector3_t){mesh->vertices[i].x, mesh->vertices[i].y, mesh->vertices[i].z}); /* affine transform so can use this faster multiply */
      vector3_t vec3_transformed_vertex = {
        .x = transformed_vertex.x,
        .y = transformed_vertex.y,
        .z = transformed_vertex.z
      };
      array_push(object->transformed_vertices, vec3_transformed_vertex);
    }
  }  

  num_objects++;

  return object;
}


void bind_texture_to_object(object_t *object, texture_t *texture){
  object->texture = texture;
}