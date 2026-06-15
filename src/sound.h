#ifndef SOUND_H
#define SOUND_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <stdint.h>

#define MAX_NUM_SOUNDS_PER_ENTRY 4
#define SOUND_MAX_NUM_CHANNELS 32

#define SOUND_CHANNEL_AMBIENT_NATURE_DAY   0
#define SOUND_CHANNEL_AMBIENT_NATURE_NIGHT 1
#define SOUND_CHANNEL_AMBIENT_NATURE_CAVE  2
#define SOUND_CHANNEL_AMBIENT_WATER        3
#define SOUND_CHANNEL_AMBIENT_LAVA         4

#define SOUND_BLOCKS_CHANNEL_START          5
#define SOUND_CHANNEL_WALKING_DIRT          5
#define SOUND_CHANNEL_WALKING_SAND          6
#define SOUND_CHANNEL_WALKING_WOOD          7
#define SOUND_CHANNEL_WALKING_STONE         8

#define SOUND_CHANNEL_MINING                9

typedef struct{
  char *filenames[MAX_NUM_SOUNDS_PER_ENTRY];
  Mix_Chunk *sounds[MAX_NUM_SOUNDS_PER_ENTRY];
  int32_t num_sounds;
} sound_t;

typedef enum{
  SOUND_TYPE_NONE,

  SOUND_TYPE_AMBIENT_NATURE_DAY,
  SOUND_TYPE_AMBIENT_NATURE_NIGHT,
  SOUND_TYPE_AMBIENT_NATURE_CAVE,
  SOUND_TYPE_AMBIENT_WATER,
  SOUND_TYPE_AMBIENT_UNDERWATER,
  SOUND_TYPE_AMBIENT_LAVA,
  SOUND_TYPE_AMBIENT_WIND,

  SOUND_TYPE_WALKING_DIRT,
  SOUND_TYPE_WALKING_SAND,
  SOUND_TYPE_WALKING_WOOD,
  SOUND_TYPE_WALKING_STONE,

  SOUND_TYPE_MINING_DIRT,
  SOUND_TYPE_MINING_STONE,
  SOUND_TYPE_MINING_WOOD,
  SOUND_TYPE_MINING_LEAVES,
  SOUND_TYPE_MINING_GLASS,
} sound_type_t;

bool init_audio(void);
void shutdown_audio(void);

void play_ambient_sound(sound_type_t sound_type, int32_t channel, int32_t loop);

void play_block_mining_sound(sound_type_t sound_type);

void fadeout_transient_sound_channels();
void fadeout_ambient_sound_channel(int32_t channel_number, int32_t ms);

#endif