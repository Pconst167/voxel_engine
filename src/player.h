#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "vector.h"

typedef struct{
  vector3_t position;
  vector3_t velocity;
  vector3_t direction;
  float theta;
  float eta;
  float radius;
  float height;
  bool grounded;
} player_t;

extern player_t player;
extern vector3_t player_direction_orthogonal_left_right;

#endif