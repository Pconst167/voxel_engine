#ifndef MATERIAL_H
#define MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include "color.h"
#include "texture.h"

#define DEFAULT_ALPHA_CUTOFF 128

typedef enum{
  BLEND_MODE_OPAQUE,
  BLEND_MODE_ALPHA_TEST,
  BLEND_MODE_ALPHA_BLEND
} blend_mode_t;

typedef struct{
  char material_name[32];
  // color and light
  color_t base_color;
  color_t emissive_color;   //  object's own emission color */
  float opacity;            // used as a global alpha blending - makes object totally opaque or ghostly */
  bool two_sided_lighting;  // object will be lit on both sides of light equally */
  bool reflects_light;
  bool dither;              // individual faces can be dithered
  // alpha
  blend_mode_t blend_mode;
  uint8_t alpha_cutoff;
  // texture data
  texture_t *texture;
} material_t;

material_t generate_default_material(const char *name);

#endif