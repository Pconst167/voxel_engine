#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "color.h"

#define FPS 120
#define FRAME_TARGET_TIME (1000 / FPS)

extern int fps;

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
