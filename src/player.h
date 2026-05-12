#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "vector.h"
#include "aabb.h"

#define PLAYER_GRAVITY             22.0f
#define PLAYER_JUMP_VELOCITY        8.0f
#define PLAYER_MAX_FALL_SPEED      40.0f
#define PLAYER_GROUND_PROBE_SPEED   2.0f
#define COLLISION_EPSILON           0.001f

typedef struct{
  vector3_t position;
  vector3_t velocity;
  vector3_t direction;
  float theta;
  float eta;
  float radius;
  float height;
  aabb_3d_t aabb;
  bool on_ground;
} player_t;

extern player_t player;
extern vector3_t player_direction_orthogonal_left_right;

void update_player_aabb(void);

#endif