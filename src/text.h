#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define MAX_TEXT_LEN 256

extern TTF_Font *font;
extern char text[MAX_TEXT_LEN];

void init_font(char *font_name, int size);
void draw_text(SDL_Renderer* renderer, int x, int y, char* text);