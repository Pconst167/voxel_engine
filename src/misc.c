#include <math.h>
#include <string.h>
#include "misc.h"

float fmax2(float f0, float f1){
  if(f0 >= f1)
    return f0;
  else 
    return f1;
}

float fmin2(float f0, float f1){
  if(f0 <= f1)
    return f0;
  else
    return f1;
}

float fmax3(float f0, float f1, float f2){
  if(f0 > f1 && f0 > f2) 
    return f0;
  else if(f1 > f0 && f1 > f2) 
    return f1;
  else 
    return f2;
}

float fmin3(float f0, float f1, float f2){
  if(f0 < f1 && f0 < f2) 
    return f0;
  else if(f1 < f0 && f1 < f2) 
    return f1;
  else 
    return f2;
}

float q_rsqrt(float number) {
  float y = number;
  float x2 = y * 0.5f;
  uint32_t i;

  memcpy(&i, &y, sizeof(i));
  i = 0x5f3759df - (i >> 1);
  memcpy(&y, &i, sizeof(y));

  y = y * (1.5f - (x2 * y * y));
  return y;
}

float fpower(float base, int exp){
  float result = 1.0f;
  for(int i = 0; i < exp; i++){
    result *= base;
  }
  return result;
}

// hash based random number - each seed give a different random number
uint32_t rand_i(uint32_t seed){
  seed = (seed << 13) ^ seed;
  return ((seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7FFFFFFF);
}

// stateful random generator
// can have multiple seeds/states - just pass state as a pointer to the function and keep
// state variables somewhere else
uint32_t xorshift32(void){
  static uint32_t state = 123456789; // must not be 0

  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;

  return x;
}

float square_wave(float f){
  return (f - floorf(f)) < 0.5f;
}