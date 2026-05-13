#ifndef AABB_H
#define AABB_H

#include <stdbool.h>
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

bool aabb_3d_overlaps(const aabb_3d_t *a, const aabb_3d_t *b);
bool aabb_3d_overlaps_xz(const aabb_3d_t *aabb, const aabb_3d_t *voxel_aabb);
bool aabb_3d_overlaps_xy(const aabb_3d_t *aabb, const aabb_3d_t *voxel_aabb);
bool aabb_3d_overlaps_yz(const aabb_3d_t *aabb, const aabb_3d_t *voxel_aabb);

#endif