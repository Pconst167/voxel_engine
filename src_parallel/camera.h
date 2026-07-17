#ifndef CAMERA_H
#define CAMERA_H

#include "vector.h"

#define ETA_MAX (M_PI_2 - 0.001)
#define CAMERA_PLAYER_HEIGHT_PERCENTAGE (0.92f)

typedef struct{
  vector3_t position;
  vector3_t direction;
  float theta; /* camera rotation around y axis */
  float eta;   /* camera rotation around x axis */
} camera_t;

extern camera_t camera;

#endif