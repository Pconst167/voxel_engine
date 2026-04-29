#ifndef OBJECT_H
#define OBJECT_H

#include "mesh.h"
#include "vector.h"
#include "color.h"
#include "texture.h"
#include "aabb.h"

#define MAX_NUM_OBJECTS 1000

typedef enum{
  OBJECT_TYPE_NORMAL,
  OBJECT_TYPE_SKYBOX /* skyboxes are special objects that track camera movement so they are always at constant distance from camera */
} object_type_t;

typedef struct{
  mesh_t *mesh;
  vector3_t *transformed_vertices; /* if object is fixed, then we hold a copy of the original vertices here, only with baked transformations */
  object_type_t object_type;
  bool fixed; /* object is static and does not change */
  bool backface_culling;
  /* bounding box/sphere - for static objects with baked in transforms (bbox not present for now) */
  aabb_3d_t bbox_aabb; /* object's bounding box aabb */
  vector3_t bounding_center;
  float bsphere_radius;
  /* current object's trandformation state */
  vector3_t scale;
  vector3_t rotation;
  vector3_t translation;
  /* these are added to the current transformation state for potential movement */
  vector3_t velocity;
  vector3_t angular_velocity;
  /* an array indicating the frame id where each object face was last drawn - used to avoid redrawing the same face in different chunks */
  uint32_t *face_draw_stamp;
} object_t;

/* used to sort objects for rendering */
typedef struct{
  uint32_t object_index;
  float sort_z;
} object_sort_key_t;

extern object_t objects[MAX_NUM_OBJECTS];
extern uint32_t num_objects;

int32_t create_object(
  mesh_t *mesh,
  object_type_t object_type,
  bool fixed,
  bool backface_culling,
  bool attach_collision_aabb,
  vector3_t collision_aabb_scale,
  vector3_t collision_aabb_translation,

  vector3_t scale,
  vector3_t rotation,
  vector3_t translation,
  vector3_t velocity,
  vector3_t angular_velocity
);

aabb_3d_t get_object_aabb(uint32_t object_id);


#endif