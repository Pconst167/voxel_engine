#ifndef PLAYER_TOOLS_H
#define PLAYER_TOOLS_H

#include <stdint.h>

typedef enum{
  PLAYER_TOOL_CLASS_PICKAXE,
  PLAYER_TOOL_CLASS_AXE,
  PLAYER_TOOL_CLASS_SHOVEL,
  PLAYER_TOOL_CLASS_WEAPON
} player_tool_class_t;

typedef enum{
  PLAYER_TOOL_WOODEN_PICKAXE,
  PLAYER_TOOL_STONE_PICKAXE,
  PLAYER_TOOL_IRON_PICKAXE,
  PLAYER_TOOL_DIAMOND_PICKAXE,

  PLAYER_TOOL_WOODEN_AXE,
  PLAYER_TOOL_STONE_AXE,
  PLAYER_TOOL_IRON_AXE,
  PLAYER_TOOL_DIAMOND_AXE,

  PLAYER_TOOL_WOODEN_SHOVEL,
  PLAYER_TOOL_STONE_SHOVEL,
  PLAYER_TOOL_IRON_SHOVEL,
  PLAYER_TOOL_DIAMOND_SHOVEL,

  PLAYER_TOOL_WOODEN_SWORD,
  PLAYER_TOOL_STONE_SWORD,
  PLAYER_TOOL_IRON_SWORD,
  PLAYER_TOOL_DIAMOND_SWORD
} player_tool_type_t;

typedef struct{
  player_tool_class_t tool_class;
  const char *name;
  uint32_t mining_power;
  uint32_t attack_damage;
  uint32_t durability_max;
} player_tool_definition_t;

typedef struct{
  player_tool_type_t tool_type;
  uint32_t object_id;
  uint8_t durability;
} player_tool_t;

#endif