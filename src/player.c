#include "player.h"

player_t player = {
  .position                     = {.x = 100.0f, .y = 116.0f, .z = 100.0f},
  .velocity                     = {.x = 0.0f, .y = 0.0f, .z = 0.0f},
  .direction                    = {.x = 0.0f, .y = 0.0f, .z = 1.0f},
  .theta                        = M_PI_2,
  .eta                          = 0.0f,
  .radius                       = 0.30f,
  .height                       = 1.80f,
  .aabb                         = {0},
  .on_ground                    = false,
  .block_inventory_active_index = 0,
  .block_inventory              = {{0}},
  .block_inventory_size         = 0
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
  for(uint32_t block_inventory_index = 0; block_inventory_index < player.block_inventory_size; block_inventory_index++){
    if(player.block_inventory[block_inventory_index].block_type == block_type){
      player.block_inventory[block_inventory_index].number++;
      return;
    }
  }

  // if player does not hold that block type, then add new entry to blocks holding list and set it to 1
  // create new entry into blocks holding list
  player.block_inventory[player.block_inventory_size].block_type = block_type;
  player.block_inventory[player.block_inventory_size].number = 1;
  player.block_inventory_size++;
}

void player_use_block(){
  if(player.block_inventory[player.block_inventory_active_index].number > 0){
    player.block_inventory[player.block_inventory_active_index].number--;
  }
}

void player_set_block_inventory_index(uint32_t block_inventory_index){
  if(player.block_inventory[block_inventory_index].number > 0)
    player.block_inventory_active_index = block_inventory_index;
}