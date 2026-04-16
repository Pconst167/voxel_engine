#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "color.h"

#define FPS 100
#define FRAME_TARGET_TIME (1000 / FPS)


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
  float ortho_half_width;
  float ortho_half_height;
} projection_options_t;
extern int fps;
extern projection_options_t projection_options;

extern SDL_Window *window;
extern SDL_Renderer *renderer;

extern color_t *color_buffer;
extern SDL_Texture *color_buffer_texture;
extern float *zbuffer;

extern int window_width;
extern int window_height;
extern int total_buffer_pixels;

extern uint8_t color_buffer_reset; /* repeat byte for color buffer reset */

bool initialize_window(void); 
void sdl_render(void);
void destroy_window(void);

#endif
