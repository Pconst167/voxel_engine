#include <math.h>
#include "aabb.h"

aabb_3d_t calculate_3d_aabb(
  vector3_t v_a,
  vector3_t v_b,
  vector3_t v_c
){
  double x_min = fmin(fmin(v_a.x, v_b.x), v_c.x);
  double y_min = fmin(fmin(v_a.y, v_b.y), v_c.y);
  double z_min = fmin(fmin(v_a.z, v_b.z), v_c.z);
  double x_max = fmax(fmax(v_a.x, v_b.x), v_c.x);
  double y_max = fmax(fmax(v_a.y, v_b.y), v_c.y);
  double z_max = fmax(fmax(v_a.z, v_b.z), v_c.z);

  aabb_3d_t aabb = {
    .x_min = x_min,
    .y_min = y_min,
    .z_min = z_min,
    .x_max = x_max,
    .y_max = y_max,
    .z_max = z_max
  };

  return aabb;
}

bool aabb_3d_overlaps(const aabb_3d_t *a, const aabb_3d_t *b){
  return
    a->x_min < b->x_max &&
    a->x_max > b->x_min &&
    a->y_min < b->y_max &&
    a->y_max > b->y_min &&
    a->z_min < b->z_max &&
    a->z_max > b->z_min;
}

bool aabb_3d_overlaps_yz(const aabb_3d_t *aabb, const aabb_3d_t *voxel_aabb){
  return
    aabb->y_min < voxel_aabb->y_max &&
    aabb->y_max > voxel_aabb->y_min &&
    aabb->z_min < voxel_aabb->z_max &&
    aabb->z_max > voxel_aabb->z_min;
}

bool aabb_3d_overlaps_xy(const aabb_3d_t *aabb, const aabb_3d_t *voxel_aabb){
  return
    aabb->x_min < voxel_aabb->x_max &&
    aabb->x_max > voxel_aabb->x_min &&
    aabb->y_min < voxel_aabb->y_max &&
    aabb->y_max > voxel_aabb->y_min;
}

bool aabb_3d_overlaps_xz(const aabb_3d_t *aabb, const aabb_3d_t *voxel_aabb){
  return
    aabb->x_min < voxel_aabb->x_max &&
    aabb->x_max > voxel_aabb->x_min &&
    aabb->z_min < voxel_aabb->z_max &&
    aabb->z_max > voxel_aabb->z_min;
}