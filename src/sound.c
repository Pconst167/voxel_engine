#include <stdbool.h>
#include "sound.h"
#include "misc.h"
#include "array.h"
#include "log.h"


sound_t *sounds;
int32_t num_sounds;

static channel_volume_fade_t channel_volume_fades[SOUND_MAX_NUM_CHANNELS];

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
        load_sound("./assets/sound/mining/dirt_0.wav"),
        load_sound("./assets/sound/mining/dirt_1.wav"),
        load_sound("./assets/sound/mining/dirt_2.wav"),
      },
      .num_sounds = 3
    }
  );
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/mining/sand_0.wav"),
        load_sound("./assets/sound/mining/sand_1.wav"),
      },
      .num_sounds = 2
    }
  );
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/mining/wood_0.wav"),
        load_sound("./assets/sound/mining/wood_1.wav"),
        load_sound("./assets/sound/mining/wood_2.wav"),
      },
      .num_sounds = 3
    }
  );
  array_push(
    sounds,
    (sound_t){
      .sounds = {
        load_sound("./assets/sound/mining/stone_3.wav"),
        load_sound("./assets/sound/mining/stone_4.wav"),
        load_sound("./assets/sound/mining/stone_5.wav"),
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
        load_sound("./assets/sound/falling/dirt_1.wav"),
      },
      .num_sounds = 2
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
  if(sound_type == SOUND_TYPE_NONE || sounds[sound_type].num_sounds <= 0 || Mix_Playing(channel)){
    return;
  }

  uint32_t random_sound_index = xorshift32() % sounds[sound_type].num_sounds;

  if(sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_Volume(channel, MIX_MAX_VOLUME);
    Mix_PlayChannel(channel, sounds[sound_type].sounds[random_sound_index], loop);
  }
}

void play_transient_sound(int32_t channel, sound_type_t sound_type){

  if(sound_type == SOUND_TYPE_NONE || Mix_Playing(channel)) return;

  Mix_Volume(channel, MIX_MAX_VOLUME);

  uint32_t random_sound_index = xorshift32() % sounds[sound_type].num_sounds;

  if(sounds[sound_type].sounds[random_sound_index] != NULL){
    Mix_PlayChannel(channel, sounds[sound_type].sounds[random_sound_index], 0);
  }
}

void shutdown_audio(void){
  Mix_CloseAudio();
}

void set_channel_volume(int32_t channel, int32_t volume, bool max){
  if(max)
    Mix_Volume(channel, MIX_MAX_VOLUME);
  else
    Mix_Volume(channel, volume);
}

void fadeout_transient_sound_channels(){
  for(int32_t i = SOUND_TRANSIENT_CHANNELS_START; i < SOUND_MAX_NUM_CHANNELS; i++){
    Mix_FadeOutChannel(i, 500);
  }
}

void fadeout_ambient_sound_channel(int32_t channel, int32_t milliseconds){
  if(Mix_Playing(channel) && Mix_FadingChannel(channel) != MIX_FADING_OUT){
    Mix_FadeOutChannel(channel, milliseconds);
  }
}

void halt_channel(int32_t channel){
  Mix_HaltChannel(channel);
}

void fade_channel_volume_to(int32_t channel, int32_t target_volume, uint32_t duration_ms){
  target_volume = clamp_int(target_volume, 0, MIX_MAX_VOLUME);

  int32_t current_volume = Mix_Volume(channel, -1);

  for(int32_t fade_index = 0; fade_index < SOUND_MAX_NUM_CHANNELS; fade_index++){
    if(channel_volume_fades[fade_index].active && channel_volume_fades[fade_index].channel == channel){
      channel_volume_fades[fade_index].active = false;
    }
  }

  if(duration_ms == 0){
    Mix_Volume(channel, target_volume);
    return;
  }

  for(int32_t fade_index = 0; fade_index < SOUND_MAX_NUM_CHANNELS; fade_index++){
    if(!channel_volume_fades[fade_index].active){
      channel_volume_fades[fade_index].active = true;
      channel_volume_fades[fade_index].channel = channel;
      channel_volume_fades[fade_index].start_volume = current_volume;
      channel_volume_fades[fade_index].target_volume = target_volume;
      channel_volume_fades[fade_index].start_ticks = SDL_GetTicks();
      channel_volume_fades[fade_index].duration_ms = duration_ms;
      return;
    }
  }

  Mix_Volume(channel, target_volume);
}

void update_channel_volume_fades(void){
  uint32_t current_ticks = SDL_GetTicks();

  for(int32_t fade_index = 0; fade_index < SOUND_MAX_NUM_CHANNELS; fade_index++){
    channel_volume_fade_t *fade = &channel_volume_fades[fade_index];

    if(!fade->active) continue;

    uint32_t elapsed_ms = current_ticks - fade->start_ticks;

    if(elapsed_ms >= fade->duration_ms){
      Mix_Volume(fade->channel, fade->target_volume);
      fade->active = false;
      continue;
    }

    float t = (float)elapsed_ms / (float)fade->duration_ms;

    int32_t new_volume = fade->start_volume + (int32_t)((float)(fade->target_volume - fade->start_volume) * t);

    new_volume = clamp_int(new_volume, 0, MIX_MAX_VOLUME);

    Mix_Volume(fade->channel, new_volume);
  }
}