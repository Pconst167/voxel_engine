#include "display.h"
#include "log.h"
#include "color.h"
#include "options.h"
#include "debug.h"

#define ASPECT_RATIO_X 1920.0f
#define ASPECT_RATIO_Y 1200.0f

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;
color_t *color_buffer = NULL;
float *zbuffer = NULL;
SDL_Texture *color_buffer_texture = NULL;
int window_width;
int window_height;
uint32_t total_buffer_pixels;

int fps;

bool initialize_window(void){

  window_width  = 1200;
  window_height = (float)window_width * ASPECT_RATIO_Y / ASPECT_RATIO_X;

  if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0){
    print_log("Error initializing SDL.\n");
    return false;
  }

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

  SDL_DisplayMode mode;
  SDL_GetCurrentDisplayMode(0, &mode);

  window = SDL_CreateWindow(
    "Voxel Engine",
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    mode.w, mode.h,
    SDL_WINDOW_FULLSCREEN_DESKTOP
  );

  if(!window){
    print_log("[ERROR] failed to create SDL window");
    return false;
  }

  //SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if(!renderer){
    print_log("[ERROR] failed to create SDL renderer");
    return false;
  }

  SDL_RenderSetLogicalSize(renderer, window_width, window_height);
  //SDL_RenderSetIntegerScale(renderer, SDL_TRUE);
  SDL_SetRelativeMouseMode(SDL_TRUE);
  //SDL_MaximizeWindow(window);

  total_buffer_pixels = window_width * window_height;

  SDL_RendererInfo info;
  SDL_GetRendererInfo(renderer, &info);
  
  /* allocate the required memory in bytes to hold the color buffer */
  color_buffer = (color_t *)malloc(sizeof(color_t) * window_width * window_height);
  zbuffer = (float *)malloc(sizeof(float) * window_width * window_height);

  /* creating a sdl texture that is used to display the color buffer */
  color_buffer_texture = SDL_CreateTexture(
    renderer,
    SDL_PIXELFORMAT_RGBA32,
    SDL_TEXTUREACCESS_STREAMING,
    window_width,
    window_height
  );

  return true;
}

inline void sdl_render(void){
  SDL_UpdateTexture(color_buffer_texture, NULL, color_buffer, window_width * sizeof(color_t));
  SDL_RenderCopy(renderer, color_buffer_texture, NULL, NULL);
  SDL_RenderPresent(renderer);
}

void destroy_window(void){
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}