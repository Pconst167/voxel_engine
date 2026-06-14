#ifndef PLAYER_H
#define PLAYER_H

#include <stdbool.h>
#include "vector.h"
#include "aabb.h"
#include "block.h"
#include "player_tools.h"

#define PLAYER_GRAVITY             22.0f
#define PLAYER_JUMP_VELOCITY       8.0f
#define PLAYER_MAX_FALL_SPEED      40.0f
#define PLAYER_GROUND_PROBE_SPEED  2.0f
#define COLLISION_EPSILON          0.001f
#define PLAYER_XZ_ACCEL            10.0f
#define PLAYER_MAX_XZ_VELOCITY     6.0f

#define PLAYER_BLOCK_INVENTORY_MAX_SIZE 50

#define MAX_PLAYER_TOOLS_HOLDING 10

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
  bool on_water;
  bool on_quicksand;
  bool on_lava;
  bool lantern;
  // tools currently holding
  player_tool_t tools[MAX_PLAYER_TOOLS_HOLDING];
  uint32_t num_tools;
  uint8_t active_tool_index;
  // blocks currently holding
  player_block_inventory_entry_t block_inventory[PLAYER_BLOCK_INVENTORY_MAX_SIZE];
  uint32_t block_inventory_size;
  uint32_t block_inventory_active_index; // index of block type being held currently
} player_t;

extern player_t player;
extern vector3_t player_direction_orthogonal_left_right;
extern player_tool_definition_t player_tool_definitions[];

void init_player();
void update_player_aabb(void);
aabb_3d_t make_player_aabb_at_position(vector3_t position);

void player_obtain_block(block_type_t block_type);
void player_use_block();
void player_set_block_inventory_index(uint32_t block_inventory_index);

void player_obtain_tool(player_tool_type_t tool_type, uint32_t object_id);
void player_set_active_tool_index(uint32_t tool_index);

#endif