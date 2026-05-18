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
  .sort                           = true,
  .show_debug_info                = false
};