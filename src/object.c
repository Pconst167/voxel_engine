#include <stdio.h>
#include <stdlib.h>
#include "object.h"
#include "array.h"
#include "matrix.h"

object_t objects[MAX_NUM_OBJECTS];
uint32_t num_objects;

uint32_t create_object(
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

  /* if object is fixed/static, bake in all the transformations beforehand! */
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

    /* transform bbox/bsphere center and radius */
    vector4_t transformed_bounding_center = matrix4_mul_vector3(&world_matrix, mesh->bounding_center);
    object->bounding_center.x = transformed_bounding_center.x;
    object->bounding_center.y = transformed_bounding_center.y;
    object->bounding_center.z = transformed_bounding_center.z;

    float scale_max = object->scale.x;
    if(object->scale.y > scale_max) scale_max = object->scale.y;
    if(object->scale.z > scale_max) scale_max = object->scale.z;
    object->bsphere_radius = mesh->bsphere_radius * scale_max;

    /* recompute aabb bounding box after transformations */
    vector3_t bbox_min = object->transformed_vertices[0];
    vector3_t bbox_max = object->transformed_vertices[0];
    for(uint32_t i = 0; i < mesh->num_vertices; i++){
      if(object->transformed_vertices[i].x < bbox_min.x) bbox_min.x = object->transformed_vertices[i].x;
      if(object->transformed_vertices[i].y < bbox_min.y) bbox_min.y = object->transformed_vertices[i].y;
      if(object->transformed_vertices[i].z < bbox_min.z) bbox_min.z = object->transformed_vertices[i].z;
      
      if(object->transformed_vertices[i].x > bbox_max.x) bbox_max.x = object->transformed_vertices[i].x;
      if(object->transformed_vertices[i].y > bbox_max.y) bbox_max.y = object->transformed_vertices[i].y;
      if(object->transformed_vertices[i].z > bbox_max.z) bbox_max.z = object->transformed_vertices[i].z;
    }
    object->bbox_aabb.x_min = bbox_min.x;
    object->bbox_aabb.x_max = bbox_max.x;
    object->bbox_aabb.y_min = bbox_min.y;
    object->bbox_aabb.y_max = bbox_max.y;
    object->bbox_aabb.z_min = bbox_min.z;
    object->bbox_aabb.z_max = bbox_max.z;
  }

  /* create face draw stamp array */
  object->face_draw_stamp = calloc(object->mesh->num_faces, sizeof(uint32_t));

  num_objects++;

  return num_objects - 1;
}


void bind_texture_to_object(object_t *object, texture_t *texture){
  object->texture = texture;
}