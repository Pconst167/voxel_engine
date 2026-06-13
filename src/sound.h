#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdint.h>
#include "player_tools.h"
#include "block.h"

#define MAX_NUM_SOUNDS_PER_BLOCK 4

typedef enum{
  BLOCK_SOUND_TYPE_DIRT,
  BLOCK_SOUND_TYPE_STONE
} block_sound_type_t;

typedef struct{
  char *filename[MAX_NUM_SOUNDS_PER_BLOCK];
  int32_t num_sounds;
} block_sound_t;

bool init_audio(void);
void shutdown_audio(void);

void play_block_mining_sound(block_type_t block_type);
void play_block_breaking_sound(block_type_t block_type);

#endif