#include <stdint.h>
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