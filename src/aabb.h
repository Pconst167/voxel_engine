#ifndef AABB_H
#define AABB_H

#include "vector.h"

typedef struct{
  double x_min, y_min, z_min;
  double x_max, y_max, z_max;
} aabb_3d_t;

aabb_3d_t calculate_3d_aabb(
  vector3_t v_a,
  vector3_t v_b,
  vector3_t v_c
);


#endif