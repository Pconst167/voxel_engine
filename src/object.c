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

void load_png_texture_data(object_t *object, char *filename){
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

  object->texture_width  = width;
  object->texture_height = height;

  object->texture = (color_t *)malloc(width * height * sizeof(color_t));
  if(object->texture == NULL){
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
    object->texture[i] = (color_t){
      .rgb.alpha = a,
      .rgb.blue  = b,
      .rgb.green = g,
      .rgb.red   = r
    };
  }

  upng_free(png_texture);
}

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

  char *texture_filename
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

  load_png_texture_data(&objects[num_objects], texture_filename);

  num_objects++;

  return &objects[num_objects - 1];
}

