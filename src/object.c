#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"
#include "array.h"
#include "matrix.h"
#include "collision.h"
#include "log.h"
#include "debug.h"

object_t objects[MAX_NUM_OBJECTS];
uint32_t num_objects;

int32_t create_object(
  mesh_t *mesh,
  object_type_t object_type,
  bool fixed,
  bool backface_culling,
  bool attach_collision_aabb,
  bool collision_aabb_active,
  vector3_t collision_aabb_scale,
  vector3_t collision_aabb_translation,

  vector3_t scale,
  vector3_t rotation,
  vector3_t translation,
  vector3_t velocity,
  vector3_t angular_velocity
){
  if(num_objects >= MAX_NUM_OBJECTS){
    print_log("ERROR: max number of objects reached %d", MAX_NUM_OBJECTS);
    return -1;
  }

  print_log("creating new object: %d", num_objects);

  object_t *object = &objects[num_objects];

  object->mesh                    = mesh;
  object->object_type             = object_type;
  object->fixed                   = fixed;
  object->backface_culling        = backface_culling;
                                  
  object->scale                   = scale;
  object->rotation                = rotation;
  object->translation             = translation;
  object->velocity                = velocity;
  object->angular_velocity        = angular_velocity;
 
  // set the material_overrides array to NULL
  object->material_overrides      = NULL;
  // allocate space for the material override flags, because these flags are tested every time to see if an object has overrides
  // and therefore the array cannot be empty
  object->material_override_flags = malloc(sizeof(bool) * mesh->num_materials);
  memset(object->material_override_flags, 0x00, sizeof(bool) * mesh->num_materials);

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

    // scale the bounding sphere radius according to object's scale
    float scale_max = object->scale.x;
    if(object->scale.y > scale_max) scale_max = object->scale.y;
    if(object->scale.z > scale_max) scale_max = object->scale.z;
    object->bsphere_radius = mesh->bsphere_radius * scale_max;

    /* compute object's aabb bounding box after vertex transformations - cannot be the same bbox as the mesh's because object can be scaled by transforms */
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

    // create and attach a collision aabb to object
    if(attach_collision_aabb){
      create_collision_aabb(
        (aabb_3d_t){
          .x_min = 0.5 * object->bbox_aabb.x_min * (1.0 + collision_aabb_scale.x) + 0.5 * object->bbox_aabb.x_max * (1.0 - collision_aabb_scale.x) + collision_aabb_translation.x,
          .x_max = 0.5 * object->bbox_aabb.x_min * (1.0 - collision_aabb_scale.x) + 0.5 * object->bbox_aabb.x_max * (1.0 + collision_aabb_scale.x) + collision_aabb_translation.x,
          .y_min = 0.5 * object->bbox_aabb.y_min * (1.0 + collision_aabb_scale.y) + 0.5 * object->bbox_aabb.y_max * (1.0 - collision_aabb_scale.y) + collision_aabb_translation.y,
          .y_max = 0.5 * object->bbox_aabb.y_min * (1.0 - collision_aabb_scale.y) + 0.5 * object->bbox_aabb.y_max * (1.0 + collision_aabb_scale.y) + collision_aabb_translation.y,
          .z_min = 0.5 * object->bbox_aabb.z_min * (1.0 + collision_aabb_scale.z) + 0.5 * object->bbox_aabb.z_max * (1.0 - collision_aabb_scale.z) + collision_aabb_translation.z,
          .z_max = 0.5 * object->bbox_aabb.z_min * (1.0 - collision_aabb_scale.z) + 0.5 * object->bbox_aabb.z_max * (1.0 + collision_aabb_scale.z) + collision_aabb_translation.z
        },
        num_objects,
        collision_aabb_active
      );
      print_log(
        "created and attached new (%s) collision aabb to object: %d (x_min: %.3f, x_max: %.3f, z_min: %.3f, z_max: %.3f, y_min: %.3f, y_max: %.3f)", 
        collision_aabb_active ? "active" : "inactive",
        num_objects,
        object->bbox_aabb.x_min, object->bbox_aabb.x_max,
        object->bbox_aabb.y_min, object->bbox_aabb.y_max,
        object->bbox_aabb.z_min, object->bbox_aabb.z_max
      );
    }
  }

  /* create face draw stamp array */
  object->face_draw_stamp = calloc(object->mesh->num_faces, sizeof(uint32_t));

  num_objects++;

  return num_objects - 1;
}

aabb_3d_t get_object_aabb(uint32_t object_id){
  return objects[object_id].bbox_aabb;
}

void object_activate_collision_aabb(uint32_t collision_aabb_id){
  collision_aabbs[collision_aabb_id].active = true;
}

void object_deactivate_collision_aabb(uint32_t collision_aabb_id){
  collision_aabbs[collision_aabb_id].active = false;
}

void object_override_material(uint32_t object_id, char *material_name, material_t new_material){
  if(object_id >= num_objects){
    print_log("ERROR: object index out of bounds: %u", object_id);
    return;
  }

  object_t *object = &objects[object_id];

  mesh_t *mesh = object->mesh;
  if(mesh == NULL){
    print_log("ERROR: referenced mesh in object does not exist");
    return;
  }

  uint32_t num_mesh_materials = mesh->num_materials;
  for(uint32_t material_i = 0; material_i < num_mesh_materials; material_i++){
    if(!strcmp(mesh->materials[material_i].material_name, material_name)){
      if(object->material_overrides == NULL){
        object->material_overrides = malloc(sizeof(material_t) * num_mesh_materials);
        memset(object->material_overrides, 0x00, sizeof(material_t) * num_mesh_materials);
      }

      object->material_overrides[material_i] = new_material;
      object->material_override_flags[material_i] = true;
      return;
    }
  }

  print_log("ERROR: material not found when trying to add override: %s", material_name);
}

uint32_t object_get_material_id_by_name(uint32_t object_id, char *material_name){
  object_t *object = &objects[object_id];

  uint32_t num_obj_materials = object->mesh->num_materials;
  for(uint32_t material_i = 0; material_i < num_obj_materials; material_i++){
    if(!strcmp(object->material_overrides[material_i].material_name, material_name)){
      return material_i;
    }
  }
  print_log("ERROR: override material '%s' not found in object %u", material_name, object_id);
  return -1;
}