#include "player.h"

player_t player = {
  .position                   = {.x = 100.0f, .y = 116.0f, .z = 100.0f},
  .velocity                   = {.x = 0.0f, .y = 0.0f, .z = 0.0f},
  .direction                  = {.x = 0.0f, .y = 0.0f, .z = 1.0f},
  .theta                      = M_PI_2,
  .eta                        = 0.0f,
  .radius                     = 0.30f,
  .height                     = 1.80f,
  .aabb                       = {0},
  .on_ground                  = false,
  .active_block_holding_index = 0,
  .blocks_holding             = {{0}},
  .num_blocks_holding_entries = 0
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

// check if player already has the block type, if so add +1 to entry
// if player is not holding that type, add new entry
void player_obtain_block(voxel_type_t block_type){
  // check if player currently holds that block type and add to list if so
  for(uint32_t holding_index = 0; holding_index < player.num_blocks_holding_entries; holding_index++){
    if(player.blocks_holding[holding_index].block_type == block_type){
      player.blocks_holding[holding_index].number_holding++;
      return;
    }
  }

  // if player does not hold that block type, then add new entry to blocks holding list and set it to 1
  // create new entry into blocks holding list
  player.blocks_holding[player.num_blocks_holding_entries].block_type = block_type;
  player.blocks_holding[player.num_blocks_holding_entries].number_holding = 1;
  player.num_blocks_holding_entries++;
}

void player_uses_block(){
  if(player.blocks_holding[player.active_block_holding_index].number_holding > 0){
    player.blocks_holding[player.active_block_holding_index].number_holding--;
  }
}

void player_select_block_type(uint32_t block_holding_index){
  player.active_block_holding_index = block_holding_index;
}