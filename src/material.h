#ifndef MATERIAL_H
#define MATERIAL_H

#include <stdint.h>
#include <stdbool.h>
#include "color.h"
#include "texture.h"

typedef enum{
  BLEND_MODE_OPAQUE,
  BLEND_MODE_ALPHA_TEST,
  BLEND_MODE_ALPHA_BLEND
} blend_mode_t;

typedef struct{
  /* color and light */
  color_t base_color;
  color_t emissive_color; /* object's own emission color */
  float emissive_intensity; /* if object is self-luminous, how emissive? */
  float opacity; /* used as a global alpha blending - makes object totally opaque or ghostly */
  bool two_sided_lighting; /* object will be lit on both sides of light equally */
  bool reflective;
  bool specular;
  blend_mode_t blend_mode;
  uint8_t alpha_cutoff;
  /* texture data */
  texture_t *texture;
} material_t;

#endif