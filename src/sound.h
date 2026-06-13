#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdint.h>

#define MAX_NUM_SOUNDS_PER_BLOCK 4
#define MAX_NUM_AMBIENT_SOUNDS_PER_ENTRY 2

typedef enum{
  AMBIENT_SOUND_TYPE_NONE,
  AMBIENT_SOUND_TYPE_NATURE,
  AMBIENT_SOUND_TYPE_NATURE_NIGHT,
  AMBIENT_SOUND_TYPE_WATER,
  AMBIENT_SOUND_TYPE_WALKING,
  AMBIENT_SOUND_TYPE_LAVA,
  AMBIENT_SOUND_TYPE_WIND,
} ambient_sound_type_t;

typedef struct{
  char *filenames[MAX_NUM_AMBIENT_SOUNDS_PER_ENTRY];
  Mix_Chunk *sounds[MAX_NUM_AMBIENT_SOUNDS_PER_ENTRY];
  int32_t num_sounds;
} ambient_sound_t;

typedef enum{
  BLOCK_SOUND_TYPE_NONE,
  BLOCK_SOUND_TYPE_DIRT,
  BLOCK_SOUND_TYPE_STONE,
  BLOCK_SOUND_TYPE_WOOD,
  BLOCK_SOUND_TYPE_LEAVES,
  BLOCK_SOUND_TYPE_GLASS,
} block_sound_type_t;

typedef struct{
  char *filenames[MAX_NUM_SOUNDS_PER_BLOCK];
  Mix_Chunk *sounds[MAX_NUM_SOUNDS_PER_BLOCK];
  int32_t num_sounds;
} block_sound_t;

bool init_audio(void);
void shutdown_audio(void);

void play_ambient_sound(ambient_sound_type_t sound_type, int32_t channel);

void play_block_mining_sound(block_sound_type_t sound_type);
void play_block_breaking_sound(block_sound_type_t sound_type);

void fadeout_sound_channels();
void fadeout_ambient_sound_channel(int32_t channel_number, int32_t ms);

#endif