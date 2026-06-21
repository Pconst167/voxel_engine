#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdbool.h>

typedef enum{
  TEXT_PERSPECTIVE_CORRECT, TEXT_AFFINE, TEXT_FILLED, TEXT_WIREFRAME, TEXT_ORTHOGRAPHIC
} projection_mode_t;

typedef struct{
  projection_mode_t projection_mode;
  bool orthographic;
  bool wireframe_overlay;
  bool backface_culling_override;
  bool backface_culling;
  bool dither;
  bool object_render_debug;
  bool render_chunk_outlines;
  bool render_collision_aabb_outlines;
  bool log;
  bool chunk_sort;
  bool show_debug_info;
  bool instant_mining;
  bool see_through;
  bool fly;
} options_t;

extern options_t options;

void set_backface_culling_override_on(bool backface_culling);
void set_backface_culling_override_off();
  
#endif