#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include <stdbool.h>

#ifndef M_PI
  #define M_PI 3.14159265358979323846
#endif
#define M_2_PI (2.0 * M_PI)
#define M_PI_2 (M_PI / 2.0)
#define M_PI_4 (M_PI / 4.0)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define DITHER_QUANTIZATION 15
#define DITHER_QUANTIZATION_POWER 4

float fmax2(float f0, float f1);
float fmin2(float f0, float f1);

float fmax3(float f0, float f1, float f2);
float fmin3(float f0, float f1, float f2);

float q_rsqrt(float number);

float fpower(float base, int exp);

uint32_t rand_i(uint32_t seed);
uint32_t xorshift32(void);

float square_wave(float f);

int clamp_int(int value, int minimum_value, int maximum_value);

bool mod_range(int x, int min, int max);

#endif