#include "options.h"

options_t options = {
  .wireframe_overlay              = false, 
  .projection_mode                = TEXT_PERSPECTIVE_CORRECT,
  .backface_culling_override      = false,
  .backface_culling               = true,
  .dither                         = false,
  .render_chunk_outlines          = false,
  .render_collision_aabb_outlines = false,
  .log                            = true,
  .chunk_sort                     = false,
  .show_debug_info                = false,
  .instant_mining                 = false
};

void set_backface_culling_override_on(bool backface_culling){
  options.backface_culling = backface_culling;
  options.backface_culling_override = true;
}

void set_backface_culling_override_off(){
  options.backface_culling_override = false;
}