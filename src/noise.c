#include <math.h>
#include <stdint.h>
#include "noise.h"

float fade(float t){
  return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float lerp_float(float t, float a, float b){
  return a + t * (b - a);
}

float grad_3d(uint32_t hash, float x, float y, float z){
  switch(hash & 15u){
    case 0:  return  x + y;
    case 1:  return -x + y;
    case 2:  return  x - y;
    case 3:  return -x - y;
    case 4:  return  x + z;
    case 5:  return -x + z;
    case 6:  return  x - z;
    case 7:  return -x - z;
    case 8:  return  y + z;
    case 9:  return -y + z;
    case 10: return  y - z;
    case 11: return -y - z;
    case 12: return  x + y;
    case 13: return -x + y;
    case 14: return -y + z;
    case 15: return -y - z;
  }

  return 0.0f;
}

uint32_t hash_3d(int32_t x, int32_t y, int32_t z){
  uint32_t hash = 2166136261u;

  hash ^= (uint32_t)x;
  hash *= 16777619u;

  hash ^= (uint32_t)y;
  hash *= 16777619u;

  hash ^= (uint32_t)z;
  hash *= 16777619u;

  hash ^= hash >> 13;
  hash *= 1274126177u;
  hash ^= hash >> 16;

  return hash;
}

float perlin_noise_3d(float x, float y, float z){
  int32_t x0 = (int32_t)floorf(x);
  int32_t y0 = (int32_t)floorf(y);
  int32_t z0 = (int32_t)floorf(z);

  int32_t x1 = x0 + 1;
  int32_t y1 = y0 + 1;
  int32_t z1 = z0 + 1;

  float xf = x - (float)x0;
  float yf = y - (float)y0;
  float zf = z - (float)z0;

  float u = fade(xf);
  float v = fade(yf);
  float w = fade(zf);

  float n000 = grad_3d(hash_3d(x0, y0, z0), xf,      yf,      zf);
  float n100 = grad_3d(hash_3d(x1, y0, z0), xf - 1,  yf,      zf);
  float n010 = grad_3d(hash_3d(x0, y1, z0), xf,      yf - 1,  zf);
  float n110 = grad_3d(hash_3d(x1, y1, z0), xf - 1,  yf - 1,  zf);

  float n001 = grad_3d(hash_3d(x0, y0, z1), xf,      yf,      zf - 1);
  float n101 = grad_3d(hash_3d(x1, y0, z1), xf - 1,  yf,      zf - 1);
  float n011 = grad_3d(hash_3d(x0, y1, z1), xf,      yf - 1,  zf - 1);
  float n111 = grad_3d(hash_3d(x1, y1, z1), xf - 1,  yf - 1,  zf - 1);

  float x00 = lerp_float(u, n000, n100);
  float x10 = lerp_float(u, n010, n110);
  float x01 = lerp_float(u, n001, n101);
  float x11 = lerp_float(u, n011, n111);

  float y0_mix = lerp_float(v, x00, x10);
  float y1_mix = lerp_float(v, x01, x11);

  float result = lerp_float(w, y0_mix, y1_mix);

  /*
    Scale into a comfortable approximate -1.0f to +1.0f range.
  */
  result *= 0.7f;

  if(result < -1.0f) result = -1.0f;
  if(result >  1.0f) result =  1.0f;

  return result;
}