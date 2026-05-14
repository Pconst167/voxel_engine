#include "player.h"

player_t player = {
  .position       = {.x = 100.0f, .y = 43.0f, .z = 100.0f},
  .velocity       = {.x = 0.0f, .y = 0.0f, .z = 0.0f},
  .direction      = {.x = 0.0f, .y = 0.0f, .z = 1.0f},
  .theta          = M_PI_2,
  .eta            = 0.0f,
  .radius         = 0.30f,
  .height         = 1.95f,
  .aabb           = {0},
  .on_ground      = false
};

vector3_t player_direction_orthogonal_left_right;

void update_player_aabb(void){
  player.aabb.x_min = player.position.x - player.radius;
  player.aabb.x_max = player.position.x + player.radius;

  player.aabb.y_min = player.position.y;
  player.aabb.y_max = player.position.y + player.height;

  player.aabb.z_min = player.position.z - player.radius;
  player.aabb.z_max = player.position.z + player.radius;
}

aabb_3d_t make_player_aabb_at_position(vector3_t position){
  aabb_3d_t aabb;

  aabb.x_min = position.x - player.radius;
  aabb.x_max = position.x + player.radius;

  aabb.y_min = position.y;
  aabb.y_max = position.y + player.height;

  aabb.z_min = position.z - player.radius;
  aabb.z_max = position.z + player.radius;

  return aabb;
}