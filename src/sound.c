#include <stdbool.h>
#include "sound.h"
#include "misc.h"
#include "log.h"

ambient_sound_t ambient_sounds[] = {
  {
    .filenames = {0},
    .num_sounds = 0
  },

  {
    .filenames = {
      "./assets/sound/nature.wav",
    },
    .num_sounds = 1
  },

  {
    .filenames = {
      "./assets/sound/nature_night.wav",
    },
    .num_sounds = 1
  },

  {
    .filenames = {
      "./assets/sound/water_1.wav",
    },
    .num_sounds = 1
  },

  {
    .filenames = {
      "./assets/sound/walk_grass.wav",
    },
    .num_sounds = 1
  },

};

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
  },

  {
    .filenames = {
      "./assets/sound/wood_0.wav",
      "./assets/sound/wood_1.wav",
    },
    .num_sounds = 2
  }
};

int32_t num_ambient_sounds = 5;
int32_t num_block_sounds = 4;

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
        print_log("[ERROR] failed to load block sound: %s\n", Mix_GetError());
      }
    }
  }

  for(int32_t sound_i = 0; sound_i < num_ambient_sounds; sound_i++){
    for(int32_t sound_element_i = 0; sound_element_i < ambient_sounds[sound_i].num_sounds; sound_element_i++){
      ambient_sounds[sound_i].sounds[sound_element_i] = Mix_LoadWAV(ambient_sounds[sound_i].filenames[sound_element_i]);
      if(ambient_sounds[sound_i].sounds[sound_element_i] == NULL){
        print_log("[ERROR] failed to load ambient sound: %s\n", Mix_GetError());
      }
    }
  }

  return true;
}

void play_ambient_sound(ambient_sound_type_t sound_type, int32_t channel){
  if(sound_type == AMBIENT_SOUND_TYPE_NONE){
    return;
  }

  if(ambient_sounds[sound_type].num_sounds <= 0){
    return;
  }

  if(Mix_Playing(channel) && Mix_FadingChannel(channel) != MIX_FADING_OUT){
    return;
  }

  if(Mix_FadingChannel(channel) == MIX_FADING_OUT){
    Mix_HaltChannel(channel);
  }

  uint32_t random_sound_index = xorshift32() % ambient_sounds[sound_type].num_sounds;

  if(ambient_sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_Volume(channel, MIX_MAX_VOLUME);
    Mix_PlayChannel(channel, ambient_sounds[sound_type].sounds[random_sound_index], -1);
  }
}

void play_block_mining_sound(block_sound_type_t sound_type){

  if(sound_type == BLOCK_SOUND_TYPE_NONE) return;

  uint32_t random_sound_index = xorshift32() % block_sounds[sound_type].num_sounds;

  int32_t channel_number = Mix_PlayChannel(-1, block_sounds[sound_type].sounds[random_sound_index], 0);

  if(channel_number >= 0){
    Mix_Volume(channel_number, MIX_MAX_VOLUME);
  }

  if(block_sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_PlayChannel(-1, block_sounds[sound_type].sounds[random_sound_index], 0);
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

void fadeout_sound_channels(){
  for(int32_t i = 4; i < 32; i++){
    Mix_FadeOutChannel(i, 500);
  }
}

void fadeout_ambient_sound_channel(int32_t channel_number, int32_t ms){
  if(Mix_Playing(channel_number)){
    Mix_FadeOutChannel(channel_number, ms);
  }
}