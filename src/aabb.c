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