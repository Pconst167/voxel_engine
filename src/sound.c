#include <stdbool.h>
#include "sound.h"
#include "misc.h"
#include "log.h"

sound_t sounds[] = {
  [SOUND_TYPE_NONE] = {
    .filenames = {0},
    .num_sounds = 0
  },

  // AMBIENT
  [SOUND_TYPE_AMBIENT_NATURE_DAY] = {
    .filenames = {
      "./assets/sound/ambient/nature_day.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_AMBIENT_NATURE_NIGHT] = {
    .filenames = {
      "./assets/sound/ambient/nature_night.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_AMBIENT_NATURE_CAVE] = {
    .filenames = {
      "./assets/sound/ambient/nature_cave.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_AMBIENT_WATER] = {
    .filenames = {
      "./assets/sound/ambient/water_1.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_AMBIENT_UNDERWATER] = {
    .filenames = {
      "./assets/sound/ambient/underwater.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_AMBIENT_LAVA] = {
    .filenames = {
      "./assets/sound/ambient/lava.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_AMBIENT_WIND] = {
    .filenames = {
      "./assets/sound/ambient/wind.wav",
    },
    .num_sounds = 1
  },

  // WALKING
  [SOUND_TYPE_WALKING_DIRT] = {
    .filenames = {
      "./assets/sound/walking/walk_dirt.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_WALKING_SAND] = {
    .filenames = {
      "./assets/sound/walking/walk_sand.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_WALKING_WOOD] = {
    .filenames = {
      "./assets/sound/walking/walk_wood.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_WALKING_STONE] = {
    .filenames = {
      "./assets/sound/walking/walk_stone.wav",
    },
    .num_sounds = 1
  },

  // BLOCK MINING
  [SOUND_TYPE_MINING_DIRT] = {
    .filenames = {
      "./assets/sound/block_mining/dirt_0.wav",
      "./assets/sound/block_mining/dirt_1.wav",
      "./assets/sound/block_mining/dirt_2.wav",
    },
    .num_sounds = 3
  },
  [SOUND_TYPE_MINING_SAND] = {
    .filenames = {0},
    .num_sounds = 0
  },
  [SOUND_TYPE_MINING_WOOD] = {
    .filenames = {
      "./assets/sound/block_mining/wood_0.wav",
      "./assets/sound/block_mining/wood_1.wav",
      "./assets/sound/block_mining/wood_2.wav",
    },
    .num_sounds = 3
  },
  [SOUND_TYPE_MINING_STONE] = {
    .filenames = {
      "./assets/sound/block_mining/stone_3.wav",
      "./assets/sound/block_mining/stone_4.wav",
      "./assets/sound/block_mining/stone_5.wav",
    },
    .num_sounds = 3
  },
  [SOUND_TYPE_MINING_LEAVES] = {
    .filenames = {0},
    .num_sounds = 0
  },
  [SOUND_TYPE_MINING_GLASS] = {
    .filenames = {0},
    .num_sounds = 0
  },


  // FALLING
  [SOUND_TYPE_FALLING_DIRT] = {
    .filenames = {
      "./assets/sound/falling/dirt_0.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_FALLING_SAND] = {
    .filenames = {
      "./assets/sound/falling/sand_0.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_FALLING_WOOD] = {
    .filenames = {
      "./assets/sound/falling/wood_0.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_FALLING_STONE] = {
    .filenames = {
      "./assets/sound/falling/stone_0.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_FALLING_WATER] = {
    .filenames = {
      "./assets/sound/falling/water_soft.wav",
    },
    .num_sounds = 1
  },
  [SOUND_TYPE_FALLING_WATER_STRONG] = {
    .filenames = {
      "./assets/sound/falling/water_strong.wav",
    },
    .num_sounds = 1
  },
};

int32_t num_sounds = 24;

bool init_audio(void){
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
    print_log("[ERROR] Mix_OpenAudio failed: %s\n", Mix_GetError());
    return false;
  }

  Mix_AllocateChannels(SOUND_MAX_NUM_CHANNELS);

  for(int32_t sound_i = 0; sound_i < num_sounds; sound_i++){
    for(int32_t sound_element_i = 0; sound_element_i < sounds[sound_i].num_sounds; sound_element_i++){
      sounds[sound_i].sounds[sound_element_i] = Mix_LoadWAV(sounds[sound_i].filenames[sound_element_i]);
      if(sounds[sound_i].sounds[sound_element_i] == NULL){
        print_log("[ERROR] failed to load block sound: %s (%s)", sounds[sound_i].filenames[sound_element_i], Mix_GetError());
      }
      else{
        print_log("[INFO] loaded sound: %s", sounds[sound_i].filenames[sound_element_i]);
      }
    }
  }

  return true;
}

void play_ambient_sound(sound_type_t sound_type, int32_t channel, int32_t loop){
  if(sound_type == SOUND_TYPE_NONE ||
     sounds[sound_type].num_sounds <= 0 ||
     Mix_Playing(channel)){
    return;
  }

  uint32_t random_sound_index = xorshift32() % sounds[sound_type].num_sounds;

  if(sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_Volume(channel, MIX_MAX_VOLUME);
    Mix_PlayChannel(channel, sounds[sound_type].sounds[random_sound_index], loop);
  }
}

void play_mining_sound(sound_type_t sound_type){

  if(sound_type == SOUND_TYPE_NONE) return;
  if(Mix_Playing(SOUND_CHANNEL_MINING)) return;

  uint32_t random_sound_index = xorshift32() % sounds[sound_type].num_sounds;

  Mix_Volume(SOUND_CHANNEL_MINING, MIX_MAX_VOLUME);

  if(sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_PlayChannel(SOUND_CHANNEL_MINING, sounds[sound_type].sounds[random_sound_index], 0);
  }
}

void play_falling_sound(sound_type_t sound_type){

  if(sound_type == SOUND_TYPE_NONE) return;
  if(Mix_Playing(SOUND_CHANNEL_FALLING)) return;

  uint32_t random_sound_index = xorshift32() % sounds[sound_type].num_sounds;

  Mix_Volume(SOUND_CHANNEL_FALLING, MIX_MAX_VOLUME);

  if(sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_PlayChannel(SOUND_CHANNEL_FALLING, sounds[sound_type].sounds[random_sound_index], 0);
  }
}

void shutdown_audio(void){
  Mix_CloseAudio();
}

void fadeout_transient_sound_channels(){
  for(int32_t i = SOUND_BLOCKS_CHANNEL_START; i < SOUND_MAX_NUM_CHANNELS; i++){
    Mix_FadeOutChannel(i, 500);
  }
}

void fadeout_ambient_sound_channel(int32_t channel, int32_t milliseconds){
  if(Mix_Playing(channel) && Mix_FadingChannel(channel) != MIX_FADING_OUT){
    Mix_FadeOutChannel(channel, milliseconds);
  }
}