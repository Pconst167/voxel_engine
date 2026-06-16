#include <stdbool.h>
#include "sound.h"
#include "misc.h"
#include "array.h"
#include "log.h"

sound_t *sounds;
int32_t num_sounds;

bool init_audio(void){
  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
    print_log("[ERROR] Mix_OpenAudio failed: %s\n", Mix_GetError());
    return false;
  }

  Mix_AllocateChannels(SOUND_MAX_NUM_CHANNELS);

  // ALLOCATE SOUND ENTRIES
  array_push(
    sounds,
    (sound_t){
      .sounds = {0},
      .num_sounds = 0
    }
  );

  // AMBIENT
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/nature_day.wav"),
      },
      .num_sounds = 1
    }
  );

  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/nature_night.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/nature_cave.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/water_1.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/underwater.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/lava.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/ambient/wind.wav"),
      },
      .num_sounds = 1
    }
  );

  // WALKING
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/walking/walk_dirt.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/walking/walk_sand.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/walking/walk_wood.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/walking/walk_stone.wav"),
      },
      .num_sounds = 1
    }
  );

  // BLOCK MINING
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/block_mining/dirt_0.wav"),
        load_sound("./assets/sound/block_mining/dirt_1.wav"),
        load_sound("./assets/sound/block_mining/dirt_2.wav"),
      },
      .num_sounds = 3
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {0},
      .num_sounds = 0
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/block_mining/wood_0.wav"),
        load_sound("./assets/sound/block_mining/wood_1.wav"),
        load_sound("./assets/sound/block_mining/wood_2.wav"),
      },
      .num_sounds = 3
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/block_mining/stone_3.wav"),
        load_sound("./assets/sound/block_mining/stone_4.wav"),
        load_sound("./assets/sound/block_mining/stone_5.wav"),
      },
      .num_sounds = 3
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {0},
      .num_sounds = 0
    }
  );

  
  array_push(
    sounds,
    (sound_t){
      .sounds = {0},
      .num_sounds = 0
    }
  );


  // FALLING
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/falling/dirt_0.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/falling/sand_0.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/falling/wood_0.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/falling/stone_0.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/falling/water_soft.wav"),
      },
      .num_sounds = 1
    }
  );
  
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/falling/water_strong.wav"),
      },
      .num_sounds = 1
    }
  );

  num_sounds = array_length(sounds);

  return true;
}

Mix_Chunk* load_sound(char *filename){
  Mix_Chunk *chunk = Mix_LoadWAV(filename);

  if(chunk == NULL){
    print_log("[ERROR] failed to load block sound: %s (%s)", filename, Mix_GetError());
    return NULL;
  }
  else{
    print_log("[INFO] loaded sound: %s", filename);
    return chunk;
  }
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

void play_transient_sound(int32_t channel, sound_type_t sound_type){

  if(sound_type == SOUND_TYPE_NONE) return;
  if(Mix_Playing(channel)) return;

  uint32_t random_sound_index = xorshift32() % sounds[sound_type].num_sounds;

  Mix_Volume(channel, MIX_MAX_VOLUME);

  if(sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_PlayChannel(channel, sounds[sound_type].sounds[random_sound_index], 0);
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