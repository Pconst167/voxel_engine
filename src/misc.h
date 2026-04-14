#ifndef MISC_H
#define MISC_H

#define M_PI 3.14159265358979323846
#define M_2_PI (2.0f * M_PI)
#define M_PI_2 (M_PI / 2.0)
#define M_PI_4 (M_PI / 4.0)

float fmax2(float f0, float f1);
float fmin2(float f0, float f1);

float fmax3(float f0, float f1, float f2);
float fmin3(float f0, float f1, float f2);

float q_rsqrt(float number);

float fpower(float base, int exp);

#endif