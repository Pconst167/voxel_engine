#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

extern TTF_Font *font;

void init_font(char *font_name, int size);
void draw_text(SDL_Renderer* renderer, int x, int y, char* text);