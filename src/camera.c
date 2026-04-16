#include "vector.h"
#include "camera.h"
#include "misc.h"

camera_t camera = {
  .position  = {0, 6, -100}, 
  .direction = {0, 0, 1},
  .theta     = M_PI_2,
  .eta       = 0.0
};

vector3_t camera_direction_orthogonal_left_right;