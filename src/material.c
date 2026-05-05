#include <stdio.h>
#include <string.h>
#include "material.h"

// create a default material
material_t generate_default_material(const char *name){
  material_t material;
  memset(&material, 0, sizeof(material));

  if(name)
    snprintf(material.material_name, sizeof(material.material_name), "%s", name);
  else
    snprintf(material.material_name, sizeof(material.material_name), "default");

  material.base_color          = color_from_float_rgb(1.0f, 1.0f, 1.0f);
  material.emissive_color      = color_from_float_rgb(0.0f, 0.0f, 0.0f);
  material.opacity             = 1.0f;
  material.two_sided_lighting  = false;
  material.reflects_light      = true;
  material.specular_highlights = false;
  material.dither              = false;
  material.blend_mode          = BLEND_MODE_OPAQUE;
  material.alpha_cutoff        = DEFAULT_ALPHA_CUTOFF;
  material.texture             = NULL;

  return material;
}