#ifndef AUDIO_H
#define AUDIO_H

#include <stdbool.h>
#include <SDL2/SDL_mixer.h>

bool audio_init(void);
void audio_shutdown(void);

bool audio_load_background(const char *filename);
void audio_play_background(int loops);
void audio_stop_background(void);
void audio_set_background_volume(int volume);

#endif