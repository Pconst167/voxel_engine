#include "player.h"

player_t player = {
  .position       = {.x = 0.0f, .y = 0.0f, .z = 0.0f},
  .velocity       = {.x = 0.0f, .y = 0.0f, .z = 0.0f},
  .direction      = {.x = 0.0f, .y = 0.0f, .z = 1.0f},
  .theta          = M_PI_2,
  .eta            = 0.0f,
  .radius         = 8.0f,
  .height         = 160.0f,
  .grounded       = false
};

vector3_t player_direction_orthogonal_left_right;