#ifndef NOISE_H
#define NOISE_H

float fade(float t);
float lerp_float(float t, float a, float b);
float grad_3d(uint32_t hash, float x, float y, float z);
uint32_t hash_3d(int32_t x, int32_t y, int32_t z);
float perlin_noise_3d(float x, float y, float z);

#endif