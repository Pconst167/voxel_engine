#include "object.h"

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
  objects[num_objects].mesh               = mesh;
  objects[num_objects].object_type        = object_type;
  objects[num_objects].backface_culling   = backface_culling;
                                          
  objects[num_objects].solid_color        = solid_color;
  objects[num_objects].emissive_color     = emissive_color;
  objects[num_objects].emissive_intensity = emissive_intensity;
  objects[num_objects].opacity            = opacity;
  objects[num_objects].two_sided_lighting = two_sided_lighting;
  objects[num_objects].reflective         = reflective;
  objects[num_objects].specular           = specular;

  objects[num_objects].blend_mode         = blend_mode;
  objects[num_objects].alpha_cutoff       = alpha_cutoff;
                                          
  objects[num_objects].scale              = scale;
  objects[num_objects].rotation           = rotation;
  objects[num_objects].translation        = translation;
  objects[num_objects].velocity           = velocity;
  objects[num_objects].angular_velocity   = angular_velocity;

  objects[num_objects].texture            = texture;

  num_objects++;

  return &objects[num_objects - 1];
}


void bind_texture_to_object(object_t *object, texture_t *texture){
  object->texture = texture;
}