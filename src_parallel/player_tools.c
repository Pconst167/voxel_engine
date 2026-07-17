#include "player_tools.h"

player_tool_definition_t player_tool_definitions[] = {
  [PLAYER_TOOL_WOODEN_PICKAXE] = {
    .tool_class = PLAYER_TOOL_CLASS_PICKAXE,
    .name = "Wooden Pickaxe",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_PICKAXE] = {
    .tool_class = PLAYER_TOOL_CLASS_PICKAXE,
    .name = "Stone Pickaxe",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_PICKAXE] = {
    .tool_class = PLAYER_TOOL_CLASS_PICKAXE,
    .name = "Iron Pickaxe",
    .mining_power = 8,
    .attack_damage = 4,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_PICKAXE] = {
    .tool_class = PLAYER_TOOL_CLASS_PICKAXE,
    .name = "Diamond Pickaxe",
    .mining_power = 16,
    .attack_damage = 8,
    .durability_max = 255
  },

  [PLAYER_TOOL_WOODEN_AXE] = {
    .tool_class = PLAYER_TOOL_CLASS_AXE,
    .name = "Wooden Axe",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_AXE] = {
    .tool_class = PLAYER_TOOL_CLASS_AXE,
    .name = "Stone Axe",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_AXE] = {
    .tool_class = PLAYER_TOOL_CLASS_AXE,
    .name = "Iron Axe",
    .mining_power = 8,
    .attack_damage = 4,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_AXE] = {
    .tool_class = PLAYER_TOOL_CLASS_AXE,
    .name = "Diamond Axe",
    .mining_power = 16,
    .attack_damage = 8,
    .durability_max = 255
  },

  [PLAYER_TOOL_WOODEN_SHOVEL] = {
    .tool_class = PLAYER_TOOL_CLASS_SHOVEL,
    .name = "Wooden Shovel",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_SHOVEL] = {
    .tool_class = PLAYER_TOOL_CLASS_SHOVEL,
    .name = "Stone Shovel",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_SHOVEL] = {
    .tool_class = PLAYER_TOOL_CLASS_SHOVEL,
    .name = "Iron Shovel",
    .mining_power = 8,
    .attack_damage = 4,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_SHOVEL] = {
    .tool_class = PLAYER_TOOL_CLASS_SHOVEL,
    .name = "Diamond Shovel",
    .mining_power = 16,
    .attack_damage = 8,
    .durability_max = 255
  },

  [PLAYER_TOOL_WOODEN_SWORD] = {
    .tool_class = PLAYER_TOOL_CLASS_WEAPON,
    .name = "Wooden Sword",
    .mining_power = 2,
    .attack_damage = 1,
    .durability_max = 32
  },
  [PLAYER_TOOL_STONE_SWORD] = {
    .tool_class = PLAYER_TOOL_CLASS_WEAPON,
    .name = "Stone Sword",
    .mining_power = 4,
    .attack_damage = 2,
    .durability_max = 64
  },
  [PLAYER_TOOL_IRON_SWORD] = {
    .tool_class = PLAYER_TOOL_CLASS_WEAPON,
    .name = "Iron Sword",
    .mining_power = 8,
    .attack_damage = 4,
    .durability_max = 128
  },
  [PLAYER_TOOL_DIAMOND_SWORD] = {
    .tool_class = PLAYER_TOOL_CLASS_WEAPON,
    .name = "Diamond Sword",
    .mining_power = 16,
    .attack_damage = 8,
    .durability_max = 255
  },
};