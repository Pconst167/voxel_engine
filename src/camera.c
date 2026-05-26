#include "camera.h"

camera_t camera = {
  .position  = {0, 0, 0}, 
  .direction = {0, 0, 1},
  .theta     = M_PI_2,
  .eta       = 0.0f
};
