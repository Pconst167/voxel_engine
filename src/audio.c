#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "audio.h"
#include "log.h"

static Mix_Music *background_music = NULL;

bool audio_init(void){
  if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0){
    print_log("[ERROR] SDL audio init failed: %s\n", SDL_GetError());
    return false;
  }

  if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0){
    print_log("[ERROR] Mix_OpenAudio failed: %s\n", Mix_GetError());
    return false;
  }

  /* optional, but good practice */
  Mix_AllocateChannels(16);

  return true;
}

void audio_shutdown(void){
  if(background_music != NULL){
    Mix_FreeMusic(background_music);
    background_music = NULL;
  }

  Mix_CloseAudio();
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool audio_load_background(const char *filename){
  if(background_music != NULL){
    Mix_FreeMusic(background_music);
    background_music = NULL;
  }

  background_music = Mix_LoadMUS(filename);
  if(background_music == NULL){
    print_log("[ERROR] Mix_LoadMUS failed for %s: %s\n", filename, Mix_GetError());
    return false;
  }

  return true;
}

void audio_play_background(int loops){
  if(background_music == NULL) return;

  if(Mix_PlayMusic(background_music, loops) == -1){
    print_log("[ERROR] Mix_PlayMusic failed: %s\n", Mix_GetError());
  }
}

void audio_stop_background(void){
  Mix_HaltMusic();
}

void audio_set_background_volume(int volume){
  if(volume < 0) volume = 0;
  if(volume > MIX_MAX_VOLUME) volume = MIX_MAX_VOLUME;

  Mix_VolumeMusic(volume);
}