#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "vector.h"
#include "aabb.h"
#include "block.h"

#define PLAYER_GRAVITY             22.0f
#define PLAYER_JUMP_VELOCITY       8.0f
#define PLAYER_MAX_FALL_SPEED      40.0f
#define PLAYER_GROUND_PROBE_SPEED  2.0f
#define COLLISION_EPSILON          0.001f
#define PLAYER_XZ_ACCEL            10.0f
#define PLAYER_MAX_XZ_VELOCITY     6.0f

#define PLAYER_BLOCK_INVENTORY_MAX_SIZE 50

typedef struct{
  block_type_t block_type;
  uint32_t number;
} player_block_inventory_entry_t;

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

  // currently holding
  player_block_inventory_entry_t block_inventory[PLAYER_BLOCK_INVENTORY_MAX_SIZE];
  uint32_t block_inventory_size;
  uint32_t block_inventory_active_index; // index of block type being held currently
} player_t;

extern player_t player;
extern vector3_t player_direction_orthogonal_left_right;

void init_player();
void update_player_aabb(void);
aabb_3d_t make_player_aabb_at_position(vector3_t position);

void player_obtain_block(block_type_t block_type);
void player_set_block_inventory_index(uint32_t block_inventory_index);

#endif