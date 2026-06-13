#include <stdbool.h>
#include "sound.h"
#include "misc.h"
#include "log.h"

block_sound_t block_sounds[] = {
  {
    .filenames = {0},
    .num_sounds = 0
  },

  {
    .filenames = {
      "./assets/sound/dirt_0.wav",
      "./assets/sound/dirt_1.wav",
      "./assets/sound/dirt_2.wav",
      "./assets/sound/dirt_3.wav",
    },
    .num_sounds = 4
  },
  
  {
    .filenames = {
      "./assets/sound/stone_0.wav",
    },
    .num_sounds = 1
  }
};

int32_t num_block_sounds = 3;

bool init_audio(void){
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
    print_log("[ERROR] Mix_OpenAudio failed: %s\n", Mix_GetError());
    return false;
  }

  Mix_AllocateChannels(32);

  for(int32_t sound_i = 0; sound_i < num_block_sounds; sound_i++){
    for(int32_t sound_element_i = 0; sound_element_i < block_sounds[sound_i].num_sounds; sound_element_i++){
      block_sounds[sound_i].sounds[sound_element_i] = Mix_LoadWAV(block_sounds[sound_i].filenames[sound_element_i]);
      if(block_sounds[sound_i].sounds[sound_element_i] == NULL){
        print_log("[ERROR] failed to load sound: %s\n", Mix_GetError());
      }
    }

  }

  return true;
}

void play_block_mining_sound(block_sound_type_t sound_type){

  uint32_t random_sound_index = xorshift32() % block_sounds[sound_type].num_sounds;

  switch(sound_type){
    case BLOCK_SOUND_TYPE_DIRT:
      if(block_sounds[sound_type].sounds[random_sound_index] != NULL){
        Mix_PlayChannel(-1, block_sounds[sound_type].sounds[random_sound_index], 0);
      }
      break;

    case BLOCK_SOUND_TYPE_STONE:
    default:
      if(block_sounds[sound_type].sounds[random_sound_index] != NULL){
        Mix_PlayChannel(-1, block_sounds[sound_type].sounds[random_sound_index], 0);
      }

      break;
  }
}

void play_block_breaking_sound(block_sound_type_t sound_type){
  uint32_t random_sound_index = xorshift32() % 3;

  switch(sound_type){
    case BLOCK_SOUND_TYPE_DIRT:
      //if(block_breaking_sounds[random_sound_index] != NULL){
      //  Mix_PlayChannel(-1, block_breaking_sounds[random_sound_index], 0);
      //}
      break;

    case BLOCK_SOUND_TYPE_STONE:
    default:
      //if(block_breaking_sounds[random_sound_index] != NULL){
      //  Mix_PlayChannel(-1, block_breaking_sounds[random_sound_index], 0);
      //}

      break;

  }
}

void shutdown_audio(void){

  Mix_CloseAudio();
}