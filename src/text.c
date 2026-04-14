#include "text.h"

TTF_Font *font;
char text[MAX_TEXT_LEN];

/* initialize font */
void init_font(char *font_name, int size){
  if (TTF_Init() == -1) {
    fprintf(stderr, "TTF_Init failed: %s\n", TTF_GetError());
    return;
  }

  font = TTF_OpenFont(font_name, size);
  if (!font) {
    fprintf(stderr, "TTF_OpenFont failed for '%s': %s\n", font_name, TTF_GetError());
    return;
  }
}

void draw_text(SDL_Renderer* renderer, int x, int y, char* text) {
  if (!font) {
    fprintf(stderr, "draw_text: font is NULL\n");
    return;
  }

  SDL_Color color = {255, 255, 255, 255};

  SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
  if (!surface) {
    fprintf(stderr, "TTF_RenderText_Solid failed: %s\n", TTF_GetError());
    return;
  }

  SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!texture) {
    fprintf(stderr, "SDL_CreateTextureFromSurface failed: %s\n", SDL_GetError());
    SDL_FreeSurface(surface);
    return;
  }

  SDL_Rect dst = {x, y, surface->w, surface->h};
  SDL_RenderCopy(renderer, texture, NULL, &dst);

  SDL_FreeSurface(surface);
  SDL_DestroyTexture(texture);
}