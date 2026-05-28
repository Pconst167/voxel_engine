#include "player.h"
#include "block.h"

player_t player = {
  .position                     = {.x = 100.0f, .y = 105.0f, .z = 100.0f},
  .velocity                     = {.x = 0.0f, .y = 0.0f, .z = 0.0f},
  .direction                    = {.x = 0.0f, .y = 0.0f, .z = 1.0f},
  .theta                        = M_PI_2,
  .eta                          = 0.0f,
  .radius                       = 0.30f,
  .height                       = 1.80f,
  .aabb                         = {0},

  .on_ground                    = false,
  // blocks holding
  .block_inventory_active_index = 0,
  .block_inventory              = {{0}},

  .block_inventory_size         = 0,
  // tools holding
  .tools                        = {{0}},
  .num_tools                    = 0,
  .active_tool_index            = 0
};

player_tool_definition_t player_tool_definitions[] = {
  [PLAYER_TOOL_WOODEN_PICKAXE] = {
    .name = "Wooden Pickaxe",
    .mining_power = 1,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_PICKAXE] = {
    .name = "Stone Pickaxe",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_PICKAXE] = {
    .name = "Iron Pickaxe",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_PICKAXE] = {
    .name = "Diamond Pickaxe",
    .mining_power = 8,
    .attack_damage = 3,
    .durability_max = 255
  },

  [PLAYER_TOOL_WOODEN_AXE] = {
    .name = "Wooden Axe",
    .mining_power = 1,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_AXE] = {
    .name = "Stone Axe",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_AXE] = {
    .name = "Iron Axe",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_AXE] = {
    .name = "Diamond Axe",
    .mining_power = 8,
    .attack_damage = 3,
    .durability_max = 255
  },

  [PLAYER_TOOL_WOODEN_SHOVEL] = {
    .name = "Wooden Shovel",
    .mining_power = 1,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_SHOVEL] = {
    .name = "Stone Shovel",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_SHOVEL] = {
    .name = "Iron Shovel",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_SHOVEL] = {
    .name = "Diamond Shovel",
    .mining_power = 8,
    .attack_damage = 3,
    .durability_max = 255
  },

  [PLAYER_TOOL_WOODEN_SWORD] = {
    .name = "Wooden Sword",
    .mining_power = 1,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_SWORD] = {
    .name = "Stone Sword",
    .mining_power = 1,
    .attack_damage = 4,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_SWORD] = {
    .name = "Iron Sword",
    .mining_power = 1,
    .attack_damage = 8,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_SWORD] = {
    .name = "Diamond Sword",
    .mining_power = 1,
    .attack_damage = 16,
    .durability_max = 255
  },
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

void init_player(){
  player.block_inventory[0].block_type = BLOCK_TYPE_GLASS;
  player.block_inventory[0].number = 40;
  player.block_inventory[1].block_type = BLOCK_TYPE_LAMP0;
  player.block_inventory[1].number = 40;
  player.block_inventory[2].block_type = BLOCK_TYPE_OVEN0;
  player.block_inventory[2].number = 40;
  player.block_inventory[3].block_type = BLOCK_TYPE_CHEST0;
  player.block_inventory[3].number = 40;
  player.block_inventory_size = 4;
}

// check if player already has the block type, if so add +1 to entry
// if player is not holding that type, add new entry
void player_obtain_block(block_type_t block_type){
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

  player.block_inventory[player.block_inventory_active_index].number--;

  // if inventory block count reaches 0, then move all inventory block positions down by one
  if(player.block_inventory[player.block_inventory_active_index].number == 0){
    for(uint32_t block_inv_i = player.block_inventory_active_index; block_inv_i < player.block_inventory_size - 1; block_inv_i++){
      player.block_inventory[block_inv_i] = player.block_inventory[block_inv_i + 1];
      player.block_inventory[block_inv_i + 1].number = 0; // set the count of the slot above as 0 to clear any empty slots
    }
    player.block_inventory_size--;

    // now find the next inventory slot which has a block count > 0 and select it
    for(int32_t block_inv_i = player.block_inventory_active_index; block_inv_i >= 0; block_inv_i--){
      if(player.block_inventory[block_inv_i].number > 0){
        player.block_inventory_active_index = block_inv_i;
        break;
      }
    }
  }
}

void player_set_block_inventory_index(uint32_t block_inventory_index){
  if(player.block_inventory[block_inventory_index].number > 0)
    player.block_inventory_active_index = block_inventory_index;
}

void player_obtain_tool(player_tool_type_t tool_type, uint32_t object_id){
  player.tools[player.num_tools].object_id     = object_id;
  player.tools[player.num_tools].tool_type     = tool_type;
  player.tools[player.num_tools].durability    = player_tool_definitions[tool_type].durability_max;
  player.num_tools++;
}

void player_set_active_tool_index(uint32_t tool_index){
  if(tool_index < player.num_tools){
    player.active_tool_index = tool_index;
  }
}