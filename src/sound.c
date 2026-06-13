#include <stdbool.h>
#include "sound.h"
#include "misc.h"

Mix_Chunk *block_mining_sounds[4];
Mix_Chunk *block_breaking_sounds[3];

block_sound_t block_sounds[] = {
  {
    .filename = {
      "dirt_0.wav",
      "dirt_1.wav",
      "dirt_2.wav",
      "dirt_3.wav",
    },
    .num_sounds = 4
  },
  
  {
    .filename = {
      "stone_0.wav",
    },
    .num_sounds = 1
  }
};

bool init_audio(void){
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
    printf("[ERROR] Mix_OpenAudio failed: %s\n", Mix_GetError());
    return false;
  }

  Mix_AllocateChannels(32);

  block_mining_sounds[0] = Mix_LoadWAV("./assets/sound/dirt_0.wav");
  if(block_mining_sounds[0] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }
  block_mining_sounds[1] = Mix_LoadWAV("./assets/sound/dirt_1.wav");
  if(block_mining_sounds[1] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }
  block_mining_sounds[2] = Mix_LoadWAV("./assets/sound/dirt_2.wav");
  if(block_mining_sounds[2] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }
  block_mining_sounds[3] = Mix_LoadWAV("./assets/sound/dirt_3.wav");
  if(block_mining_sounds[3] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }


  block_breaking_sounds[0] = Mix_LoadWAV("./assets/sound/breaking_0.wav");
  if(block_breaking_sounds[0] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }
  block_breaking_sounds[1] = Mix_LoadWAV("./assets/sound/breaking_1.wav");
  if(block_breaking_sounds[1] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }
  block_breaking_sounds[2] = Mix_LoadWAV("./assets/sound/breaking_0.wav");
  if(block_breaking_sounds[2] == NULL){
    printf("[ERROR] Mix_LoadWAV failed: %s\n", Mix_GetError());
    return false;
  }

  return true;
}

void play_block_mining_sound(block_type_t block_type){
  uint32_t random_sound_index = xorshift32() % 4;

  switch(block_type){
    case BLOCK_TYPE_DIRT0:
    case BLOCK_TYPE_DIRT1:
      if(block_mining_sounds[random_sound_index] != NULL){
        Mix_PlayChannel(-1, block_mining_sounds[random_sound_index], 0);
      }
      break;

    case BLOCK_TYPE_STONE0:
    case BLOCK_TYPE_STONE1:
    case BLOCK_TYPE_STONE2:
    case BLOCK_TYPE_STONE3:
    case BLOCK_TYPE_STONE4:
    case BLOCK_TYPE_STONE5:
    case BLOCK_TYPE_STONE6:
    case BLOCK_TYPE_STONE7:
    case BLOCK_TYPE_STONE8:
    default:
      if(block_mining_sounds[random_sound_index] != NULL){
        Mix_PlayChannel(-1, block_mining_sounds[random_sound_index], 0);
      }

      break;

  }
}

void play_block_breaking_sound(block_type_t block_type){
  uint32_t random_sound_index = xorshift32() % 3;

  switch(block_type){
    case BLOCK_TYPE_DIRT0:
    case BLOCK_TYPE_DIRT1:
      if(block_breaking_sounds[random_sound_index] != NULL){
        Mix_PlayChannel(-1, block_breaking_sounds[random_sound_index], 0);
      }
      break;

    case BLOCK_TYPE_STONE0:
    case BLOCK_TYPE_STONE1:
    case BLOCK_TYPE_STONE2:
    case BLOCK_TYPE_STONE3:
    case BLOCK_TYPE_STONE4:
    case BLOCK_TYPE_STONE5:
    case BLOCK_TYPE_STONE6:
    case BLOCK_TYPE_STONE7:
    case BLOCK_TYPE_STONE8:
    default:
      if(block_breaking_sounds[random_sound_index] != NULL){
        Mix_PlayChannel(-1, block_breaking_sounds[random_sound_index], 0);
      }

      break;

  }
}

void shutdown_audio(void){
  if(block_mining_sounds[0] != NULL){
    Mix_FreeChunk(block_mining_sounds[0]);
    block_mining_sounds[0] = NULL;
  }
  if(block_mining_sounds[1] != NULL){
    Mix_FreeChunk(block_mining_sounds[1]);
    block_mining_sounds[1] = NULL;
  }
  if(block_mining_sounds[2] != NULL){
    Mix_FreeChunk(block_mining_sounds[2]);
    block_mining_sounds[2] = NULL;
  }

  Mix_CloseAudio();
}