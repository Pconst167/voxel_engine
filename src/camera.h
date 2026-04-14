#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"

#define ETA_MAX (M_PI_2 - 0.001)

typedef struct{
  vector3_t position;
  vector3_t direction;
  float theta; /* camera rotation around y axis */
  float eta;   /* camera rotation around x axis */
} camera_t;

extern camera_t camera;
extern vector3_t camera_direction_orthogonal_left_right;

#endif