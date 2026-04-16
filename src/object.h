#ifndef OBJECT_H
#define OBJECT_H

#include <stdio.h>
#include <stdlib.h>
#include "mesh.h"
#include "vector.h"
#include "color.h"
#include "texture.h"

#define MAX_NUM_OBJECTS 1000

typedef enum{
  OBJECT_TYPE_NORMAL,
  OBJECT_TYPE_SKYBOX /* skyboxes are special objects that track camera movement so they are always at constant distance from camera */
} object_type_t;

/* for now one object maps to one mesh. */
typedef struct{
  mesh_t *mesh;
  vector3_t *transformed_vertices; /* if object is fixed, then we hold a copy of the original vertices here, only with baked transformations */
  object_type_t object_type;
  bool fixed; /* object is static and does not change */
  bool backface_culling;
  /* bounding box/sphere - for static objects with baked in transforms (bbox not present for now) */
  vector3_t bounding_center;
  float bsphere_radius;
  /* color and light */
  color_t solid_color;
  color_t emissive_color; /* object's own emission color */
  float emissive_intensity; /* if object is self-luminous, how emissive? */
  float opacity; /* used as a global alpha blending - makes object totally opaque or ghostly */
  bool two_sided_lighting; /* object will be lit on both sides of light equally */
  bool reflective;
  bool specular;
  blend_mode_t blend_mode;
  uint8_t alpha_cutoff;
  /* current object's trandformation state */
  vector3_t scale;
  vector3_t rotation;
  vector3_t translation;
  /* these are added to the current transformation state for potential movement */
  vector3_t velocity;
  vector3_t angular_velocity;
  /* texture data */
  texture_t *texture;
} object_t;

typedef struct{
  mesh_t *mesh;
} skybox_t;

/* used to sort objects for rendering */
typedef struct{
  uint32_t object_index;
  float sort_z;
} object_sort_key_t;

extern object_t objects[MAX_NUM_OBJECTS];
extern uint32_t num_objects;

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
);

void bind_texture_to_object(object_t *object, texture_t *texture);

#endif