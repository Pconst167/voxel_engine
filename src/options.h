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
} options_t;

extern options_t options;

#endif