#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "vector.h"

typedef struct{
  vector3_t position;
  vector3_t velocity;
  float radius;
  float height;
  bool grounded;
} player_t;

#endif