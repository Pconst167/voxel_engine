#ifndef OBJECT_H
#define OBJECT_H

/*
  NOTES:
    an object has overriding materials.  so an object references a mesh. a mesh has a collection of materials,
    but an object also can have a collection of materials, such that each object can then override any of the current
    mesh's materials.
*/

#include "mesh.h"
#include "vector.h"
#include "color.h"
#include "texture.h"
#include "aabb.h"
#include "material.h"

#define MAX_NUM_OBJECTS 1000
#define MAX_LEN_OBJECT_NAME 64

typedef enum{
  OBJECT_TYPE_SKYBOX, /* skyboxes are special objects that track camera movement so they are always at constant distance from camera */
  OBJECT_TYPE_ROOM,
  OBJECT_TYPE_DOOR,
  OBJECT_TYPE_COMPUTER,
  OBJECT_TYPE_ROBOT,
  OBJECT_TYPE_BUTTON,
  OBJECT_TYPE_ITEM
} object_type_t;

typedef struct{
  char name[MAX_LEN_OBJECT_NAME];
  bool turned_on;
  texture_t *screen_texture;
  uint32_t cycles_per_frame;
} computer_data_t;

typedef struct{
  bool open;
} door_data_t;

typedef struct{
  bool pressed;
} button_data_t;

typedef struct{
  char name[MAX_LEN_OBJECT_NAME];
  bool taken;
} item_data_t;

typedef union{
  computer_data_t as_computer;
  door_data_t as_door;
  button_data_t as_button;
  item_data_t as_item;
} gameplay_data_t;

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
  // overriding materials
  material_t *material_overrides;  // array of overriding materials for the object
  bool *material_override_flags; // an override flag for each material - same number of elements as materials array
  // object's gameplay data
  gameplay_data_t gameplay_data;
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
  bool collision_aabb_active,
  vector3_t collision_aabb_scale,
  vector3_t collision_aabb_translation,

  vector3_t scale,
  vector3_t rotation,
  vector3_t translation,
  vector3_t velocity,
  vector3_t angular_velocity
);

aabb_3d_t get_object_aabb(uint32_t object_id);

void object_activate_collision_aabb(uint32_t collision_aabb_id);
void object_deactivate_collision_aabb(uint32_t collision_aabb_id);
void object_override_material(uint32_t object_id, char *material_name, material_t new_material);
uint32_t object_get_material_id_by_name(uint32_t object_id, char *material_name);

#endif