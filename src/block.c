#include <stdio.h>
#include <stdint.h>
#include <float.h>
#include <stdlib.h>
#include "vector.h"
#include "block.h"
#include "misc.h"
#include "camera.h"
#include "array.h"
#include "trees.h"
#include "ores.h"
#include "volcano.h"
#include "noise.h"
#include "log.h"

// Y slowest changing dimension, X is fastest changing
block_t *blocks;

// block currently being looked at
block_raycast_hit_t looked_at_block;

// block light queue
light_queue_t light_queue;
const int32_t neighbour_y_offsets[6] = {-1,  1,  0,  0,  0,  0};
const int32_t neighbour_z_offsets[6] = { 0,  0, -1,  1,  0,  0};
const int32_t neighbour_x_offsets[6] = { 0,  0,  0,  0, -1,  1};

block_material_t *block_materials;

void init_block_materials(){
  // AIR
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "air",
      .solid              = false,
      .mass               = 0,
      .mineable           = false,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = true,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = NULL,
      .back_texture       = NULL,
      .left_texture       = NULL,
      .right_texture      = NULL,
      .top_texture        = NULL,
      .bottom_texture     = NULL,
      .mining_sound_type  = SOUND_TYPE_NONE,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_NONE,
      .falling_sound_type = SOUND_TYPE_NONE,
    })  
  );

  // LAVA
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "lava0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_LAVA,
      .transparent_to_light = false,
      .backface_culling   = false,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .reflects_light     = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/lava0.png"),
      .back_texture       = load_texture("./assets/textures/lava0.png"),
      .left_texture       = load_texture("./assets/textures/lava0.png"),
      .right_texture      = load_texture("./assets/textures/lava0.png"),
      .top_texture        = load_texture("./assets/textures/lava0.png"),
      .bottom_texture     = load_texture("./assets/textures/lava0.png"),
      .mining_sound_type  = SOUND_TYPE_NONE,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_NONE,
      .falling_sound_type = SOUND_TYPE_NONE,
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "lava1",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_LAVA,
      .transparent_to_light = false,
      .backface_culling   = false,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .reflects_light     = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/lava1.png"),
      .back_texture       = load_texture("./assets/textures/lava1.png"),
      .left_texture       = load_texture("./assets/textures/lava1.png"),
      .right_texture       = load_texture("./assets/textures/lava1.png"),
      .top_texture        = load_texture("./assets/textures/lava1.png"),
      .bottom_texture     = load_texture("./assets/textures/lava1.png"),
      
      .mining_sound_type  = SOUND_TYPE_NONE,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_NONE,
      .falling_sound_type = SOUND_TYPE_NONE,
    })  
  );

  // VOLCANIC
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .reflects_light     = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/volcanic0.png"),
      .back_texture       = load_texture("./assets/textures/volcanic0.png"),
      .left_texture       = load_texture("./assets/textures/volcanic0.png"),
      .right_texture       = load_texture("./assets/textures/volcanic0.png"),
      .top_texture        = load_texture("./assets/textures/volcanic0.png"),
      .bottom_texture     = load_texture("./assets/textures/volcanic0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic1",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .reflects_light     = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/volcanic1.png"),
      .back_texture       = load_texture("./assets/textures/volcanic1.png"),
      .left_texture       = load_texture("./assets/textures/volcanic1.png"),
      .right_texture       = load_texture("./assets/textures/volcanic1.png"),
      .top_texture        = load_texture("./assets/textures/volcanic1.png"),
      .bottom_texture     = load_texture("./assets/textures/volcanic1.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic2",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/volcanic2.png"),
      .back_texture       = load_texture("./assets/textures/volcanic2.png"),
      .left_texture       = load_texture("./assets/textures/volcanic2.png"),
      .right_texture       = load_texture("./assets/textures/volcanic2.png"),
      .top_texture        = load_texture("./assets/textures/volcanic2.png"),
      .bottom_texture     = load_texture("./assets/textures/volcanic2.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic3",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/volcanic3.png"),
      .back_texture       = load_texture("./assets/textures/volcanic3.png"),
      .left_texture       = load_texture("./assets/textures/volcanic3.png"),
      .right_texture      = load_texture("./assets/textures/volcanic3.png"),
      .top_texture        = load_texture("./assets/textures/volcanic3.png"),
      .bottom_texture     = load_texture("./assets/textures/volcanic3.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })  
  );

  // BEDROCK
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "bedrock",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = false,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/bedrock.png"),
      .back_texture       = load_texture("./assets/textures/bedrock.png"),
      .left_texture       = load_texture("./assets/textures/bedrock.png"),
      .right_texture      = load_texture("./assets/textures/bedrock.png"),
      .top_texture        = load_texture("./assets/textures/bedrock.png"),
      .bottom_texture     = load_texture("./assets/textures/bedrock.png"),
      
      .mining_sound_type  = SOUND_TYPE_NONE,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_NONE,
      .falling_sound_type = SOUND_TYPE_NONE,
    })
  );

  // STONE
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone0.png"),
      .back_texture       = load_texture("./assets/textures/stone0.png"),
      .left_texture       = load_texture("./assets/textures/stone0.png"),
      .right_texture      = load_texture("./assets/textures/stone0.png"),
      .top_texture        = load_texture("./assets/textures/stone0.png"),
      .bottom_texture     = load_texture("./assets/textures/stone0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone1",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone1.png"),
      .back_texture       = load_texture("./assets/textures/stone1.png"),
      .left_texture       = load_texture("./assets/textures/stone1.png"),
      .right_texture       = load_texture("./assets/textures/stone1.png"),
      .top_texture        = load_texture("./assets/textures/stone1.png"),
      .bottom_texture     = load_texture("./assets/textures/stone1.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone2",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone2.png"),
      .back_texture       = load_texture("./assets/textures/stone2.png"),
      .left_texture       = load_texture("./assets/textures/stone2.png"),
      .right_texture       = load_texture("./assets/textures/stone2.png"),
      .top_texture        = load_texture("./assets/textures/stone2.png"),
      .bottom_texture     = load_texture("./assets/textures/stone2.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone3",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone3.png"),
      .back_texture       = load_texture("./assets/textures/stone3.png"),
      .left_texture       = load_texture("./assets/textures/stone3.png"),
      .right_texture      = load_texture("./assets/textures/stone3.png"),
      .top_texture        = load_texture("./assets/textures/stone3.png"),
      .bottom_texture     = load_texture("./assets/textures/stone3.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone4",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone4.png"),
      .back_texture       = load_texture("./assets/textures/stone4.png"),
      .left_texture       = load_texture("./assets/textures/stone4.png"),
      .right_texture       = load_texture("./assets/textures/stone4.png"),
      .top_texture        = load_texture("./assets/textures/stone4.png"),
      .bottom_texture     = load_texture("./assets/textures/stone4.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone5",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone5.png"),
      .back_texture       = load_texture("./assets/textures/stone5.png"),
      .left_texture       = load_texture("./assets/textures/stone5.png"),
      .right_texture       = load_texture("./assets/textures/stone5.png"),
      .top_texture        = load_texture("./assets/textures/stone5.png"),
      .bottom_texture     = load_texture("./assets/textures/stone5.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone6",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone6.png"),
      .back_texture       = load_texture("./assets/textures/stone6.png"),
      .left_texture       = load_texture("./assets/textures/stone6.png"),
      .right_texture       = load_texture("./assets/textures/stone6.png"),
      .top_texture        = load_texture("./assets/textures/stone6.png"),
      .bottom_texture     = load_texture("./assets/textures/stone6.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone7",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone7.png"),
      .back_texture       = load_texture("./assets/textures/stone7.png"),
      .left_texture       = load_texture("./assets/textures/stone7.png"),
      .right_texture       = load_texture("./assets/textures/stone7.png"),
      .top_texture        = load_texture("./assets/textures/stone7.png"),
      .bottom_texture     = load_texture("./assets/textures/stone7.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone8",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/stone8.png"),
      .back_texture       = load_texture("./assets/textures/stone8.png"),
      .left_texture       = load_texture("./assets/textures/stone8.png"),
      .right_texture       = load_texture("./assets/textures/stone8.png"),
      .top_texture        = load_texture("./assets/textures/stone8.png"),
      .bottom_texture     = load_texture("./assets/textures/stone8.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // COAL
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "coal",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/coal_side.png"),
      .back_texture       = load_texture("./assets/textures/coal_side.png"),
      .left_texture       = load_texture("./assets/textures/coal_side.png"),
      .right_texture       = load_texture("./assets/textures/coal_side.png"),
      .top_texture        = load_texture("./assets/textures/coal_top.png"),
      .bottom_texture     = load_texture("./assets/textures/coal_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // IRON
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "iron",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/iron_side.png"),
      .back_texture       = load_texture("./assets/textures/iron_side.png"),
      .left_texture       = load_texture("./assets/textures/iron_side.png"),
      .right_texture       = load_texture("./assets/textures/iron_side.png"),
      .top_texture        = load_texture("./assets/textures/iron_top.png"),
      .bottom_texture     = load_texture("./assets/textures/iron_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // DIAMOND
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "diamond",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/diamond_side.png"),
      .back_texture       = load_texture("./assets/textures/diamond_side.png"),
      .left_texture       = load_texture("./assets/textures/diamond_side.png"),
      .right_texture       = load_texture("./assets/textures/diamond_side.png"),
      .top_texture        = load_texture("./assets/textures/diamond_top.png"),
      .bottom_texture     = load_texture("./assets/textures/diamond_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // SILVER
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "silver",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/silver_side.png"),
      .back_texture       = load_texture("./assets/textures/silver_side.png"),
      .left_texture       = load_texture("./assets/textures/silver_side.png"),
      .right_texture       = load_texture("./assets/textures/silver_side.png"),
      .top_texture        = load_texture("./assets/textures/silver_top.png"),
      .bottom_texture     = load_texture("./assets/textures/silver_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // GOLD
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "gold",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/gold_side.png"),
      .back_texture       = load_texture("./assets/textures/gold_side.png"),
      .left_texture       = load_texture("./assets/textures/gold_side.png"),
      .right_texture       = load_texture("./assets/textures/gold_side.png"),
      .top_texture        = load_texture("./assets/textures/gold_top.png"),
      .bottom_texture     = load_texture("./assets/textures/gold_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // DIRT
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "grasslandsdirt",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/grasslands_dirt.png"),
      .back_texture       = load_texture("./assets/textures/grasslands_dirt.png"),
      .left_texture       = load_texture("./assets/textures/grasslands_dirt.png"),
      .right_texture      = load_texture("./assets/textures/grasslands_dirt.png"),
      .top_texture        = load_texture("./assets/textures/grasslands_dirt.png"),
      .bottom_texture     = load_texture("./assets/textures/grasslands_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "forestdirt",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/forest_dirt.png"),
      .back_texture       = load_texture("./assets/textures/forest_dirt.png"),
      .left_texture       = load_texture("./assets/textures/forest_dirt.png"),
      .right_texture      = load_texture("./assets/textures/forest_dirt.png"),
      .top_texture        = load_texture("./assets/textures/forest_dirt.png"),
      .bottom_texture     = load_texture("./assets/textures/forest_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );

  // GRASSLANDS GRASS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "grass",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/grasslands_grass_side.png"),
      .back_texture       = load_texture("./assets/textures/grasslands_grass_side.png"),
      .left_texture       = load_texture("./assets/textures/grasslands_grass_side.png"),
      .right_texture      = load_texture("./assets/textures/grasslands_grass_side.png"),
      .top_texture        = load_texture("./assets/textures/grasslands_grass_top.png"),
      .bottom_texture     = load_texture("./assets/textures/grasslands_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "cherrygrass",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/grasslands_cherry_grass_side.png"),
      .back_texture       = load_texture("./assets/textures/grasslands_cherry_grass_side.png"),
      .left_texture       = load_texture("./assets/textures/grasslands_cherry_grass_side.png"),
      .right_texture      = load_texture("./assets/textures/grasslands_cherry_grass_side.png"),
      .top_texture        = load_texture("./assets/textures/grasslands_cherry_grass_top.png"),
      .bottom_texture     = load_texture("./assets/textures/grasslands_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "forestgrass",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/forest_grass_side.png"),
      .back_texture       = load_texture("./assets/textures/forest_grass_side.png"),
      .left_texture       = load_texture("./assets/textures/forest_grass_side.png"),
      .right_texture      = load_texture("./assets/textures/forest_grass_side.png"),
      .top_texture        = load_texture("./assets/textures/forest_grass_top.png"),
      .bottom_texture     = load_texture("./assets/textures/forest_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "savannahgrass",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/savannah_grass_side.png"),
      .back_texture       = load_texture("./assets/textures/savannah_grass_side.png"),
      .left_texture       = load_texture("./assets/textures/savannah_grass_side.png"),
      .right_texture      = load_texture("./assets/textures/savannah_grass_side.png"),
      .top_texture        = load_texture("./assets/textures/savannah_grass_top.png"),
      .bottom_texture     = load_texture("./assets/textures/grasslands_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );

  // MOSS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "moss",
      .solid              = true,
      .mass               = 30,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/moss0.png"),
      .back_texture       = load_texture("./assets/textures/moss0.png"),
      .left_texture       = load_texture("./assets/textures/moss0.png"),
      .right_texture      = load_texture("./assets/textures/moss0.png"),
      .top_texture        = load_texture("./assets/textures/moss0.png"),
      .bottom_texture     = load_texture("./assets/textures/moss0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_MINING_DIRT,
      .breaking_sound_type= SOUND_TYPE_MINING_DIRT,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );

  // SAND
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "sand0",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/sand0.png"),
      .back_texture       = load_texture("./assets/textures/sand0.png"),
      .left_texture       = load_texture("./assets/textures/sand0.png"),
      .right_texture      = load_texture("./assets/textures/sand0.png"),
      .top_texture        = load_texture("./assets/textures/sand0.png"),
      .bottom_texture     = load_texture("./assets/textures/sand0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_SAND,
      .placing_sound_type = SOUND_TYPE_MINING_SAND,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_SAND,
      .falling_sound_type = SOUND_TYPE_FALLING_SAND,
    })
  );

  // CLAY
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "clay0",
      .solid              = true,
      .mass               = 100,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/clay0.png"),
      .back_texture       = load_texture("./assets/textures/clay0.png"),
      .left_texture       = load_texture("./assets/textures/clay0.png"),
      .right_texture      = load_texture("./assets/textures/clay0.png"),
      .top_texture        = load_texture("./assets/textures/clay0.png"),
      .bottom_texture     = load_texture("./assets/textures/clay0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_SAND,
      .placing_sound_type = SOUND_TYPE_MINING_SAND,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_SAND,
      .falling_sound_type = SOUND_TYPE_FALLING_SAND,
    })
  );

  // SNOW
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "snowgrass",
      .solid              = true,
      .mass               = 30,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFFFFFFFF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/snow_grass_side.png"),
      .back_texture       = load_texture("./assets/textures/snow_grass_side.png"),
      .left_texture       = load_texture("./assets/textures/snow_grass_side.png"),
      .right_texture      = load_texture("./assets/textures/snow_grass_side.png"),
      .top_texture        = load_texture("./assets/textures/snow_top.png"),
      .bottom_texture     = load_texture("./assets/textures/forest_dirt.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_SAND,
      .placing_sound_type = SOUND_TYPE_MINING_SAND,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_SAND,
      .falling_sound_type = SOUND_TYPE_FALLING_SAND,
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "snow1",
      .solid              = true,
      .mass               = 30,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFFFFFFFF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/snow_1.png"),
      .back_texture       = load_texture("./assets/textures/snow_1.png"),
      .left_texture       = load_texture("./assets/textures/snow_1.png"),
      .right_texture      = load_texture("./assets/textures/snow_1.png"),
      .top_texture        = load_texture("./assets/textures/snow_1.png"),
      .bottom_texture     = load_texture("./assets/textures/snow_1.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_SAND,
      .placing_sound_type = SOUND_TYPE_MINING_SAND,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_SAND,
      .falling_sound_type = SOUND_TYPE_FALLING_SAND,
    })
  );

  // TREE TRUNKS
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk0_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk0_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk0_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk0_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk0_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk0_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk1",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk1_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk1_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk1_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk1_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk1_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk1_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk2",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk2_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk2_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk2_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk2_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk2_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk2_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk3",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk3_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk3_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk3_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk3_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk3_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk3_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk4",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk4_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk4_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk4_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk4_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk4_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk4_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk5",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk5_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk5_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk5_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk5_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk5_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk5_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk6",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk6_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk6_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk6_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk6_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk6_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk6_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk7",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/treetrunk7_side.png"),
      .back_texture       = load_texture("./assets/textures/treetrunk7_side.png"),
      .left_texture       = load_texture("./assets/textures/treetrunk7_side.png"),
      .right_texture       = load_texture("./assets/textures/treetrunk7_side.png"),
      .top_texture        = load_texture("./assets/textures/treetrunk7_top.png"),
      .bottom_texture     = load_texture("./assets/textures/treetrunk7_top.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    }
  );

  // LEAVES
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves0.png"),
      .back_texture       = load_texture("./assets/textures/leaves0.png"),
      .left_texture       = load_texture("./assets/textures/leaves0.png"),
      .right_texture       = load_texture("./assets/textures/leaves0.png"),
      .top_texture        = load_texture("./assets/textures/leaves0.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves1",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves1.png"),
      .back_texture       = load_texture("./assets/textures/leaves1.png"),
      .left_texture       = load_texture("./assets/textures/leaves1.png"),
      .right_texture       = load_texture("./assets/textures/leaves1.png"),
      .top_texture        = load_texture("./assets/textures/leaves1.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves1.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves2",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves2.png"),
      .back_texture       = load_texture("./assets/textures/leaves2.png"),
      .left_texture       = load_texture("./assets/textures/leaves2.png"),
      .right_texture       = load_texture("./assets/textures/leaves2.png"),
      .top_texture        = load_texture("./assets/textures/leaves2.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves2.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves3",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves3.png"),
      .back_texture       = load_texture("./assets/textures/leaves3.png"),
      .left_texture       = load_texture("./assets/textures/leaves3.png"),
      .right_texture       = load_texture("./assets/textures/leaves3.png"),
      .top_texture        = load_texture("./assets/textures/leaves3.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves3.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves4",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves4.png"),
      .back_texture       = load_texture("./assets/textures/leaves4.png"),
      .left_texture       = load_texture("./assets/textures/leaves4.png"),
      .right_texture       = load_texture("./assets/textures/leaves4.png"),
      .top_texture        = load_texture("./assets/textures/leaves4.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves4.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves5",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves5.png"),
      .back_texture       = load_texture("./assets/textures/leaves5.png"),
      .left_texture       = load_texture("./assets/textures/leaves5.png"),
      .right_texture       = load_texture("./assets/textures/leaves5.png"),
      .top_texture        = load_texture("./assets/textures/leaves5.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves5.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves6",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves6.png"),
      .back_texture       = load_texture("./assets/textures/leaves6.png"),
      .left_texture       = load_texture("./assets/textures/leaves6.png"),
      .right_texture       = load_texture("./assets/textures/leaves6.png"),
      .top_texture        = load_texture("./assets/textures/leaves6.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves6.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves7",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves7.png"),
      .back_texture       = load_texture("./assets/textures/leaves7.png"),
      .left_texture       = load_texture("./assets/textures/leaves7.png"),
      .right_texture       = load_texture("./assets/textures/leaves7.png"),
      .top_texture        = load_texture("./assets/textures/leaves7.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves7.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves8",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves8.png"),
      .back_texture       = load_texture("./assets/textures/leaves8.png"),
      .left_texture       = load_texture("./assets/textures/leaves8.png"),
      .right_texture       = load_texture("./assets/textures/leaves8.png"),
      .top_texture        = load_texture("./assets/textures/leaves8.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves8.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves9",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/leaves9.png"),
      .back_texture       = load_texture("./assets/textures/leaves9.png"),
      .left_texture       = load_texture("./assets/textures/leaves9.png"),
      .right_texture       = load_texture("./assets/textures/leaves9.png"),
      .top_texture        = load_texture("./assets/textures/leaves9.png"),
      .bottom_texture     = load_texture("./assets/textures/leaves9.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );

  // FLOWER LEAVES
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "flowerleaves0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/flower_leaves0.png"),
      .back_texture       = load_texture("./assets/textures/flower_leaves0.png"),
      .left_texture       = load_texture("./assets/textures/flower_leaves0.png"),
      .right_texture      = load_texture("./assets/textures/flower_leaves0.png"),
      .top_texture        = load_texture("./assets/textures/flower_leaves0.png"),
      .bottom_texture     = load_texture("./assets/textures/flower_leaves0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "cherryleaves0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/cherry_leaves0.png"),
      .back_texture       = load_texture("./assets/textures/cherry_leaves0.png"),
      .left_texture       = load_texture("./assets/textures/cherry_leaves0.png"),
      .right_texture      = load_texture("./assets/textures/cherry_leaves0.png"),
      .top_texture        = load_texture("./assets/textures/cherry_leaves0.png"),
      .bottom_texture     = load_texture("./assets/textures/cherry_leaves0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_DIRT,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_DIRT,
      .falling_sound_type = SOUND_TYPE_FALLING_DIRT,
    })
  );

  // CACTI
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "cactustrunk0",
      .solid              = true,
      .mass               = 40,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/cactus_trunk0.png"),
      .back_texture       = load_texture("./assets/textures/cactus_trunk0.png"),
      .left_texture       = load_texture("./assets/textures/cactus_trunk0.png"),
      .right_texture      = load_texture("./assets/textures/cactus_trunk0.png"),
      .top_texture        = load_texture("./assets/textures/cactus_trunk0.png"),
      .bottom_texture     = load_texture("./assets/textures/cactus_trunk0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "cactusflower0",
      .solid              = true,
      .mass               = 40,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/cactus_flower0.png"),
      .back_texture       = load_texture("./assets/textures/cactus_flower0.png"),
      .left_texture       = load_texture("./assets/textures/cactus_flower0.png"),
      .right_texture      = load_texture("./assets/textures/cactus_flower0.png"),
      .top_texture        = load_texture("./assets/textures/cactus_flower0.png"),
      .bottom_texture     = load_texture("./assets/textures/cactus_flower0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    })
  );

  // WATER
  array_push(
    block_materials,
    ((block_material_t){
      .material_name        = "water0",
      .solid                = false,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable             = true,
      .block_fluid_type     = BLOCK_FLUID_WATER,
      .transparent_to_light = true,
      .backface_culling     = false,
      .base_color           = (color_t){.uint32 = 0xFFFF8080},
      .emissive_color       = (color_t){.uint32 = 0x00000000},
      .reflects_light       = true,
      .blend_mode           = BLEND_MODE_ALPHA_BLEND,
      .alpha_cutoff         = DEFAULT_ALPHA_CUTOFF,
      .front_texture        = load_texture("./assets/textures/water0.png"),
      .back_texture         = load_texture("./assets/textures/water0.png"),
      .left_texture         = load_texture("./assets/textures/water0.png"),
      .right_texture        = load_texture("./assets/textures/water0.png"),
      .top_texture          = load_texture("./assets/textures/water0.png"),
      .bottom_texture       = load_texture("./assets/textures/water0.png"),
      
      .mining_sound_type  = SOUND_TYPE_NONE,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_NONE,
      .falling_sound_type = SOUND_TYPE_NONE,
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name        = "bog0",
      .solid                = false,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_SHOVEL,
      .mineable             = true,
      .block_fluid_type     = BLOCK_FLUID_WATER,
      .transparent_to_light = true,
      .backface_culling     = false,
      .base_color           = (color_t){.uint32 = 0xFFFF8080},
      .emissive_color       = (color_t){.uint32 = 0xFF040404},
      .reflects_light       = true,
      .blend_mode           = BLEND_MODE_ALPHA_BLEND,
      .alpha_cutoff         = DEFAULT_ALPHA_CUTOFF,
      .front_texture        = load_texture("./assets/textures/bog0.png"),
      .back_texture         = load_texture("./assets/textures/bog0.png"),
      .left_texture         = load_texture("./assets/textures/bog0.png"),
      .right_texture        = load_texture("./assets/textures/bog0.png"),
      .top_texture          = load_texture("./assets/textures/bog0.png"),
      .bottom_texture       = load_texture("./assets/textures/bog0.png"),
      
      .mining_sound_type  = SOUND_TYPE_NONE,
      .placing_sound_type = SOUND_TYPE_NONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_NONE,
      .falling_sound_type = SOUND_TYPE_NONE,
    })
  );

  // QUICKSAND

  // GLASS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "glass",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = true,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_ALPHA_TEST,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/glass0.png"),
      .back_texture       = load_texture("./assets/textures/glass0.png"),
      .left_texture       = load_texture("./assets/textures/glass0.png"),
      .right_texture       = load_texture("./assets/textures/glass0.png"),
      .top_texture        = load_texture("./assets/textures/glass0.png"),
      .bottom_texture     = load_texture("./assets/textures/glass0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // LAMPS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "lamp0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x0040FFFF},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/lamp0.png"),
      .back_texture       = load_texture("./assets/textures/lamp0.png"),
      .left_texture       = load_texture("./assets/textures/lamp0.png"),
      .right_texture       = load_texture("./assets/textures/lamp0.png"),
      .top_texture        = load_texture("./assets/textures/lamp0.png"),
      .bottom_texture     = load_texture("./assets/textures/lamp0.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    })
  );

  // OVENS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "oven0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_PICKAXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/oven_front.png"),
      .back_texture       = load_texture("./assets/textures/oven.png"),
      .left_texture       = load_texture("./assets/textures/oven_side.png"),
      .right_texture       = load_texture("./assets/textures/oven_side.png"),
      .top_texture        = load_texture("./assets/textures/oven.png"),
      .bottom_texture     = load_texture("./assets/textures/oven.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_STONE,
      .placing_sound_type = SOUND_TYPE_MINING_STONE,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_STONE,
      .falling_sound_type = SOUND_TYPE_FALLING_STONE,
    })
  );

  // CHESTS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "chest0",
      .solid              = true,
      .mass               = 150,
      .preferred_tool_class = PLAYER_TOOL_CLASS_AXE,
      .mineable           = true,
      .block_fluid_type   = BLOCK_FLUID_NONE,
      .transparent_to_light = false,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .reflects_light     = true,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/chest_front.png"),
      .back_texture       = load_texture("./assets/textures/chest_sides.png"),
      .left_texture       = load_texture("./assets/textures/chest_sides.png"),
      .right_texture       = load_texture("./assets/textures/chest_sides.png"),
      .top_texture        = load_texture("./assets/textures/chest_top_bottom.png"),
      .bottom_texture     = load_texture("./assets/textures/chest_top_bottom.png"),
      
      .mining_sound_type  = SOUND_TYPE_MINING_WOOD,
      .placing_sound_type = SOUND_TYPE_MINING_WOOD,
      .breaking_sound_type= SOUND_TYPE_NONE,
      .walking_sound_type = SOUND_TYPE_WALKING_WOOD,
      .falling_sound_type = SOUND_TYPE_FALLING_WOOD,
    })
  );

}

void allocate_block_memory(){
  blocks = malloc(sizeof(block_t) * MAP_NUM_BLOCKS_Y * MAP_NUM_BLOCKS_Z * MAP_NUM_BLOCKS_X);
  if(blocks == NULL){
    print_log("ERROR: unable to allocate map memory");
    exit(1);
  }
}

void assign_biomes(){
  biome_type_t current_biome;

  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      
      float temperature_noise = fabsf(perlin_noise_3d((float)x * WORLD_TEMPERATURE_SCALE, 0.05f,   (float)z * WORLD_TEMPERATURE_SCALE));
      float humidity_noise    = fabsf(perlin_noise_3d((float)x * WORLD_HUMIDITY_SCALE, 200.1f, (float)z * WORLD_HUMIDITY_SCALE));

      if(temperature_noise >= 0.6){
        current_biome = BIOME_TYPE_VOLCANIC;
      }
      else if(temperature_noise > 0.5 && temperature_noise < 0.6){
        current_biome = BIOME_TYPE_DESERT;
      }
      else if(temperature_noise > 0.4 && temperature_noise <= 0.5){
        if(humidity_noise <= 0.4){
          current_biome = BIOME_TYPE_DESERT;
        }
        else{
          current_biome = BIOME_TYPE_GRASSLANDS;
        }
      }
      else if(temperature_noise > 0.2 && temperature_noise <= 0.4){
        if(humidity_noise <= 0.4){
          current_biome = BIOME_TYPE_FOREST;
        }
        if(humidity_noise > 0.4 && humidity_noise <= 0.6){
          current_biome = BIOME_TYPE_GRASSLANDS;
        }
        else{
          current_biome = BIOME_TYPE_SAVANNAH;
        }
      }
      else{
        if(humidity_noise <= 0.2){
          current_biome = BIOME_TYPE_RAIN_FOREST;
        }
        else if(humidity_noise > 0.2 && humidity_noise < 0.4){
          current_biome = BIOME_TYPE_FOREST;
        }
        else if(humidity_noise >= 0.4 && humidity_noise <= 0.6){
          current_biome = BIOME_TYPE_GRASSLANDS;
        }
        else if(humidity_noise > 0.6 && humidity_noise < 0.8){
          current_biome = BIOME_TYPE_SAVANNAH;
        }
        else{
          current_biome = BIOME_TYPE_SNOW;
        }
      }

      for(int32_t y = 0; y < MAP_NUM_BLOCKS_Y; y++){
        block_at(y, z, x).biome_type = current_biome;
      }

    }
  }
}

void process_window_messages(void){
  SDL_Event event;

  while(SDL_PollEvent(&event)){
    if(event.type == SDL_QUIT){
      exit(1);
    }
  }
}

void init_block_map(){

  for(int32_t y = 0; y < MAP_NUM_BLOCKS_Y; y++){
    for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
      for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
        assign_block_material(y, z, x, BLOCK_TYPE_AIR);

        block_at(y, z, x).temperature = 30.0f;
        block_at(y, z, x).orientation = BLOCK_ORIENTATION_FRONT;
        block_at(y, z, x).sky_light   = 0;
        block_at(y, z, x).block_light = 0;

        block_at(y, z, x).aabb.x_min = x;
        block_at(y, z, x).aabb.x_max = x + 1.0f;
        block_at(y, z, x).aabb.z_min = z;
        block_at(y, z, x).aabb.z_max = z + 1.0f;
        block_at(y, z, x).aabb.y_min = y;
        block_at(y, z, x).aabb.y_max = y + 1.0f;
      }
    }
  }

  float base_world_surface_height = (float)MAP_NUM_BLOCKS_Y * 0.50f;

  // Bigger = terrain follows the spline height more strongly.
  // Smaller = 3D noise has more power, giving more holes/overhangs/floating blobs.
  float density_gradient = 0.5f;

  // Bigger = more 3D Perlin influence. i.e 0.5f to 2.0f.
  float density_noise_strength = 25.0f;

  // base world surface levels
  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      float height_noise = perlin_noise_3d((float)x * 0.01f, 100.731f, (float)z * 0.01f);
      float peaks_and_valleys_noise = perlin_noise_3d((float)x * 0.02f, 271.923f, (float)z * 0.02f);
      float weirdness_noise = fabsf(perlin_noise_3d((float)x * 0.005f, 100.231f, (float)z * 0.005f));

      // evaluate_height_spline() returns an offset around 0, for example -30 to +80.
      // So we add base_world_surface_height to give it a "DC" component.
      float height_offset = evaluate_height_spline(height_noise, peaks_and_valleys_noise);

      float surface_height = base_world_surface_height + height_offset;

      for(int32_t y = 0; y < MAP_NUM_BLOCKS_Y; y++){
        float distance_below_surface = surface_height - (float)y;
        float base_density = distance_below_surface * density_gradient;
        float density_noise = perlin_noise_3d((float)x * 0.08f, (float)y * 0.08f, (float)z * 0.08f);
        //density_noise *= density_noise;

        float final_density = base_density + density_noise * density_noise_strength * weirdness_noise;

        if(final_density > 0.0f){
          block_type_t block_type = get_random_stone();
          assign_block_material(y, z, x, block_type);
        }
      }
    }
  }

  // BEDROCK LAVA
  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      int32_t surface_y;
      float lava_noise = fabsf(perlin_noise_3d((float)x * WORLD_UNDER_NOISE_SCALE, 150.0f, (float)z * WORLD_UNDER_NOISE_SCALE));
      surface_y = WORLD_BASE_LAVA_LEVEL + (int32_t)(lava_noise * WORLD_UNDER_MAX_HEIGHT);
      for(int32_t y = surface_y; y >= 0; y--){
        assign_block_material(y, z, x, BLOCK_TYPE_LAVA0);
      }
    }
  }

  // BIOME LAYERS
  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    process_window_messages(); // avoids OS hanging
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
        block_t *block = &block_at(y, z, x);

        if(block_is_stone(block)){
          biome_type_t biome_type = block_at(0, z, x).biome_type;
          block_type_t top_surface_block, subsurface_block;
          get_biome_top_surface_block(biome_type, &top_surface_block, &subsurface_block);
          assign_block_material(y, z, x, top_surface_block);
          for(int32_t yy = y - 1; yy >= y - 1 - WORLD_DIRT0_DEPTH; yy--){
            if(block_at(yy, z, x).type == BLOCK_TYPE_AIR) break; // stop filling with we find an air block
            assign_block_material(yy, z, x, subsurface_block);
          }
          break;
        }
      }
    }
  }


  // WATER
  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    process_window_messages(); // avoids OS hanging
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
  
        biome_type_t biome_type = block_at(0, z, x).biome_type;
  
        if(y <= WORLD_WATER_LEVEL && block_at(y, z, x).type == BLOCK_TYPE_AIR){
          assign_block_material(y, z, x, BLOCK_TYPE_WATER0);
          if(z + 1 < MAP_NUM_BLOCKS_Z && block_materials[block_at(y, z + 1, x).type].solid) assign_block_material(y, z + 1, x, get_random_underwater_substrate(y, z, x));
          if(z - 1 >= 0 && block_materials[block_at(y, z - 1, x).type].solid)               assign_block_material(y, z - 1, x, get_random_underwater_substrate(y, z, x));
          if(x + 1 < MAP_NUM_BLOCKS_X && block_materials[block_at(y, z, x + 1).type].solid) assign_block_material(y, z, x + 1, get_random_underwater_substrate(y, z, x));
          if(x - 1 >= 0 && block_materials[block_at(y, z, x - 1).type].solid)               assign_block_material(y, z, x - 1, get_random_underwater_substrate(y, z, x));
          if(y - 1 >= 0 && block_materials[block_at(y - 1, z, x).type].solid)               assign_block_material(y - 1, z, x, get_random_underwater_substrate(y, z, x));
        }
      }
    }
  }

  place_trees();
  //place_ores();
}

block_type_t get_random_stone(){
  uint32_t random = xorshift32();

  if(random % 7 == 0)
    return BLOCK_TYPE_STONE0;
  else if(random % 7 == 1)
    return BLOCK_TYPE_STONE1;
  else if(random % 7 == 2)
    return BLOCK_TYPE_STONE2;
  else if(random % 7 == 3)
    return BLOCK_TYPE_STONE3;
  else if(random % 7 == 4)
    return BLOCK_TYPE_STONE4;
  else if(random % 7 == 5)
    return BLOCK_TYPE_STONE5;
  else if(random % 7 == 6)
    return BLOCK_TYPE_STONE6;

  return BLOCK_TYPE_STONE0;
}

block_type_t get_random_underwater_substrate(int32_t y, int32_t z, int32_t x){

  float substrate_noise_0 = (perlin_noise_3d((float)x * 0.1f, (float)y * 0.1f, (float)z * 0.1f));
  float substrate_noise_1 = (perlin_noise_3d((float)x * 0.05f, (float)y * 0.05f, (float)z * 0.05f));

  if(substrate_noise_0 >= 0.0f && substrate_noise_1 >= 0.0f) return BLOCK_TYPE_SAND0;
  else if(substrate_noise_0 <= 0.0f && substrate_noise_1 >= 0.0f) return BLOCK_TYPE_CLAY0;
  else if(substrate_noise_0 <= 0.0f && substrate_noise_1 <= 0.0f) return get_random_stone();

  return BLOCK_TYPE_SAND0;

}

void get_biome_top_surface_block(
  biome_type_t biome_type, 
  block_type_t *top_surface_block,
  block_type_t *subsurface_block
){

  switch(biome_type){
    case BIOME_TYPE_GRASSLANDS:
      *top_surface_block = BLOCK_TYPE_GRASSLANDS_GRASS;
      *subsurface_block = BLOCK_TYPE_GRASSLANDS_DIRT;
      return;
      
    case BIOME_TYPE_FOREST:
      *top_surface_block = BLOCK_TYPE_FOREST_GRASS;
      *subsurface_block = BLOCK_TYPE_FOREST_DIRT;
      return;
      
    case BIOME_TYPE_RAIN_FOREST:
      *top_surface_block = BLOCK_TYPE_FOREST_GRASS;
      *subsurface_block = BLOCK_TYPE_FOREST_DIRT;
      return;
      
    case BIOME_TYPE_DESERT:
      *top_surface_block = BLOCK_TYPE_SAND0;
      *subsurface_block= BLOCK_TYPE_SAND0;
      return;
      
    case BIOME_TYPE_SNOW:
      *top_surface_block = BLOCK_TYPE_SNOW_0;
      *subsurface_block= BLOCK_TYPE_SNOW_1;
      return;

    case BIOME_TYPE_VOLCANIC:
      *top_surface_block = BLOCK_TYPE_VOLCANIC3;
      *subsurface_block = BLOCK_TYPE_VOLCANIC3;
      return;

    case BIOME_TYPE_SAVANNAH:
    default:
      *top_surface_block = BLOCK_TYPE_SAVANNAH_GRASS;
      *subsurface_block = BLOCK_TYPE_GRASSLANDS_DIRT;
      return;
  }
}

void reset_light_around_area(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end){

  if(y_start < 0) y_start = 0;
  if(z_start < 0) z_start = 0;
  if(x_start < 0) x_start = 0;
  if(y_end >= MAP_NUM_BLOCKS_Y) y_end = MAP_NUM_BLOCKS_Y - 1;
  if(z_end >= MAP_NUM_BLOCKS_Z) z_end = MAP_NUM_BLOCKS_Z - 1;
  if(x_end >= MAP_NUM_BLOCKS_X) x_end = MAP_NUM_BLOCKS_X - 1;

  // UPDATE SKY LIGHT OF ALL BLOCKS
  // all blocks in open air get MAX_SKY_LIGHT/MAX_BLOCK_LIGHT light, all blocks below an opaque block get 0 light
  for(int32_t z = z_start; z <= z_end; z++){
    for(int32_t x = x_start; x <= x_end; x++){
      bool found_opaque = false;
      for(int32_t y = y_end; y >= y_start; y--){
        if(block_is_opaque(&block_at(y, z, x))) found_opaque = true;
        if(found_opaque) block_at(y, z, x).sky_light = 0;    
        else{
          block_at(y, z, x).sky_light = MAX_SKY_LIGHT; 
        }
      }
    }
  }

  // UPDATE BLOCK LIGHT OF ALL BLOCKS
  for(int32_t y = y_end; y >= y_start; y--){
    for(int32_t z = z_start; z <= z_end; z++){
      for(int32_t x = x_start; x <= x_end; x++){
        if(block_is_transparent(&block_at(y, z, x))) block_at(y, z, x).block_light = 0; 
        else if(block_materials[block_at(y, z, x).type].emissive_color.uint32 != 0x00000000) block_at(y, z, x).block_light = MAX_BLOCK_LIGHT;
      }
    }
  }
}

void seed_initial_sky_light(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end){
  light_queue.head = 0;
  light_queue.tail = 0;

  for(int32_t y = y_start; y <= y_end; y++){
    for(int32_t z = z_start; z <= z_end; z++){
      for(int32_t x = x_start; x <= x_end; x++){
        block_t *block = &block_at(y, z, x);

        if(block->sky_light != MAX_SKY_LIGHT) continue;

        for(int32_t neighbour_index = 0; neighbour_index < 6; neighbour_index++){
          int32_t neighbour_y = y + neighbour_y_offsets[neighbour_index];
          int32_t neighbour_z = z + neighbour_z_offsets[neighbour_index];
          int32_t neighbour_x = x + neighbour_x_offsets[neighbour_index];

          if(neighbour_y < 0 || neighbour_y >= MAP_NUM_BLOCKS_Y || neighbour_z < 0 || neighbour_z >= MAP_NUM_BLOCKS_Z || neighbour_x < 0 || neighbour_x >= MAP_NUM_BLOCKS_X) continue;

          block_t *neighbour = &block_at(neighbour_y, neighbour_z, neighbour_x);

          if(neighbour->sky_light == 0 && block_is_transparent(neighbour)){
            neighbour->sky_light = MAX_SKY_LIGHT - 1;
            light_queue_push((light_node_t){.y = neighbour_y, .z = neighbour_z, .x = neighbour_x, .light = MAX_SKY_LIGHT - 1});
          }
        }
      }
    }
  }
}

void seed_initial_block_light(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end){
  light_queue.head = 0;
  light_queue.tail = 0;

  for(int32_t y = y_start; y <= y_end; y++){
    for(int32_t z = z_start; z <= z_end; z++){
      for(int32_t x = x_start; x <= x_end; x++){
        block_t *block = &block_at(y, z, x);

        if(block->block_light != MAX_BLOCK_LIGHT) continue;

        for(int32_t neighbour_index = 0; neighbour_index < 6; neighbour_index++){
          int32_t neighbour_y = y + neighbour_y_offsets[neighbour_index];
          int32_t neighbour_z = z + neighbour_z_offsets[neighbour_index];
          int32_t neighbour_x = x + neighbour_x_offsets[neighbour_index];

          if(neighbour_y < 0 || neighbour_y >= MAP_NUM_BLOCKS_Y || neighbour_z < 0 || neighbour_z >= MAP_NUM_BLOCKS_Z || neighbour_x < 0 || neighbour_x >= MAP_NUM_BLOCKS_X) continue;

          block_t *neighbour = &block_at(neighbour_y, neighbour_z, neighbour_x);

          if(block_is_transparent(neighbour)){
            neighbour->block_light = MAX_BLOCK_LIGHT - 1;
            light_queue_push((light_node_t){.y = neighbour_y, .z = neighbour_z, .x = neighbour_x, .light = MAX_BLOCK_LIGHT - 1});
          }
        }
      }
    }
  }
}

void propagate_sky_light(void){
  while(light_queue_not_empty()){
    light_node_t node = light_queue_pop();

    // if node light is 1 then the next light value would be 0 anyhow, so skip it
    if(node.light <= 1) continue;

    // loop all 6 neighbours of the node
    for(int32_t neighbour_index = 0; neighbour_index < 6; neighbour_index++){
      int32_t neighbour_y = node.y + neighbour_y_offsets[neighbour_index];
      int32_t neighbour_z = node.z + neighbour_z_offsets[neighbour_index];
      int32_t neighbour_x = node.x + neighbour_x_offsets[neighbour_index];

      if(neighbour_y < 0 || neighbour_y >= MAP_NUM_BLOCKS_Y || neighbour_z < 0 || neighbour_z >= MAP_NUM_BLOCKS_Z || neighbour_x < 0 || neighbour_x >= MAP_NUM_BLOCKS_X) continue;

      // get neighbour block
      block_t *neighbour = &block_at(neighbour_y, neighbour_z, neighbour_x);

      // if block is opaque, ignore
      if(block_is_opaque(neighbour)) continue;

      // calculate light for neighbour
      uint8_t new_light = node.light - 1;

      // if neighbour's light is darker than calculated one, then push to queue
      if(new_light > neighbour->sky_light){
        neighbour->sky_light = new_light;
        light_queue_push((light_node_t){.y = neighbour_y, .z = neighbour_z, .x = neighbour_x, .light = new_light});
      }
    }
  }
}

void propagate_block_light(void){
  while(light_queue_not_empty()){
    light_node_t node = light_queue_pop();

    // if node light is 1 then the next light value would be 0 anyhow, so skip it
    if(node.light <= 1) continue;

    // loop all 6 neighbours of the node
    for(int32_t neighbour_index = 0; neighbour_index < 6; neighbour_index++){
      int32_t neighbour_y = node.y + neighbour_y_offsets[neighbour_index];
      int32_t neighbour_z = node.z + neighbour_z_offsets[neighbour_index];
      int32_t neighbour_x = node.x + neighbour_x_offsets[neighbour_index];

      if(neighbour_y < 0 || neighbour_y >= MAP_NUM_BLOCKS_Y || neighbour_z < 0 || neighbour_z >= MAP_NUM_BLOCKS_Z || neighbour_x < 0 || neighbour_x >= MAP_NUM_BLOCKS_X) continue;

      // get neighbour block
      block_t *neighbour = &block_at(neighbour_y, neighbour_z, neighbour_x);

      // if block is opaque, ignore
      if(block_is_opaque(neighbour)) continue;

      // calculate light for neighbour
      uint8_t new_light = node.light - 1;

      // if neighbour's light is darker than calculated one, then push to queue
      if(new_light > neighbour->block_light){
        neighbour->block_light = new_light;
        light_queue_push((light_node_t){.y = neighbour_y, .z = neighbour_z, .x = neighbour_x, .light = new_light});
      }
    }
  }
}

void remove_sky_light_from_area(int32_t y, int32_t z, int32_t x){
  light_queue.head = 0;
  light_queue.tail = 0;

  for(int32_t neighbour_index = 0; neighbour_index < 6; neighbour_index++){
    int32_t neighbour_y = y + neighbour_y_offsets[neighbour_index];
    int32_t neighbour_z = z + neighbour_z_offsets[neighbour_index];
    int32_t neighbour_x = x + neighbour_x_offsets[neighbour_index];

    if(neighbour_y < 0 || neighbour_y >= MAP_NUM_BLOCKS_Y || neighbour_z < 0 || neighbour_z >= MAP_NUM_BLOCKS_Z || neighbour_x < 0 || neighbour_x >= MAP_NUM_BLOCKS_X) continue;

    block_t *neighbour = &block_at(neighbour_y, neighbour_z, neighbour_x);

    if(block_is_opaque(neighbour)) continue;
    if(neighbour->sky_light == 0) continue;

    light_queue_push((light_node_t){.y = neighbour_y, .z = neighbour_z, .x = neighbour_x, .light = neighbour->sky_light});
    neighbour->sky_light = 0;
  }

  while(light_queue_not_empty()){
    light_node_t node = light_queue_pop();

    for(int32_t neighbour_index = 0; neighbour_index < 6; neighbour_index++){
      int32_t neighbour_y = node.y + neighbour_y_offsets[neighbour_index];
      int32_t neighbour_z = node.z + neighbour_z_offsets[neighbour_index];
      int32_t neighbour_x = node.x + neighbour_x_offsets[neighbour_index];

      if(neighbour_y < 0 || neighbour_y >= MAP_NUM_BLOCKS_Y || neighbour_z < 0 || neighbour_z >= MAP_NUM_BLOCKS_Z || neighbour_x < 0 || neighbour_x >= MAP_NUM_BLOCKS_X) continue;

      block_t *neighbour = &block_at(neighbour_y, neighbour_z, neighbour_x);

      if(block_is_opaque(neighbour)) continue;
      if(neighbour->sky_light == 0) continue;

      if(neighbour->sky_light < node.light){
        light_queue_push((light_node_t){.y = neighbour_y, .z = neighbour_z, .x = neighbour_x, .light = neighbour->sky_light});
        neighbour->sky_light = 0;
      }
    }
  }
}

void seed_sky_light_around_area(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end){
  light_queue.head = 0;
  light_queue.tail = 0;

  for(int32_t test_y = y_start; test_y <= y_end; test_y++){
    for(int32_t test_z = z_start; test_z <= z_end; test_z++){
      for(int32_t test_x = x_start; test_x <= x_end; test_x++){
        if(test_y < 0 || test_y >= MAP_NUM_BLOCKS_Y || test_z < 0 || test_z >= MAP_NUM_BLOCKS_Z || test_x < 0 || test_x >= MAP_NUM_BLOCKS_X) continue;

        block_t *block = &block_at(test_y, test_z, test_x);

        if(block_is_opaque(block)) continue;
        if(block->sky_light == 0) continue;

        light_queue_push((light_node_t){.y = test_y, .z = test_z, .x = test_x, .light = block->sky_light});
      }
    }
  }
}

void seed_block_light_around_area(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end){
  light_queue.head = 0;
  light_queue.tail = 0;

  for(int32_t test_y = y_start; test_y <= y_end; test_y++){
    for(int32_t test_z = z_start; test_z <= z_end; test_z++){
      for(int32_t test_x = x_start; test_x <= x_end; test_x++){
        if(test_y < 0 || test_y >= MAP_NUM_BLOCKS_Y || test_z < 0 || test_z >= MAP_NUM_BLOCKS_Z || test_x < 0 || test_x >= MAP_NUM_BLOCKS_X) continue;

        block_t *block = &block_at(test_y, test_z, test_x);

        if(block->block_light == 0) continue;

        light_queue_push((light_node_t){.y = test_y, .z = test_z, .x = test_x, .light = block->block_light});
      }
    }
  }
}

void get_block_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z){
  *start_x = clamp_int((int)floorf(aabb->x_min), 0, MAP_NUM_BLOCKS_X - 1);
  *end_x   = clamp_int((int)floorf(aabb->x_max), 0, MAP_NUM_BLOCKS_X - 1);

  *start_y = clamp_int((int)floorf(aabb->y_min), 0, MAP_NUM_BLOCKS_Y - 1);
  *end_y   = clamp_int((int)floorf(aabb->y_max), 0, MAP_NUM_BLOCKS_Y - 1);

  *start_z = clamp_int((int)floorf(aabb->z_min), 0, MAP_NUM_BLOCKS_Z - 1);
  *end_z   = clamp_int((int)floorf(aabb->z_max), 0, MAP_NUM_BLOCKS_Z - 1);
}

bool light_queue_not_empty(){
  return light_queue.head != light_queue.tail;
}

void light_queue_push(light_node_t light_node){
  light_queue.nodes[light_queue.tail] = light_node;

  light_queue.tail++;

  if(light_queue.tail >= MAX_LIGHT_QUEUE_SIZE){
    light_queue.tail = 0;
  }
}

light_node_t light_queue_pop(){
  light_node_t light_node = light_queue.nodes[light_queue.head];

  light_queue.head++;

  if(light_queue.head >= MAX_LIGHT_QUEUE_SIZE){
    light_queue.head = 0;
  }

  return light_node;
}

inline block_face_t generate_block_face(block_face_direction_t block_face_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col){
  block_face_t block_face;

  switch(block_face_type){
    case BLOCK_FACE_TOP:
      block_face.points[0].x = x_col;
      block_face.points[0].y = y_plane + 1.0f;
      block_face.points[0].z = z_row;
      block_face.points[1].x = x_col;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row + 1.0f;
      block_face.points[2].x = x_col + 1.0f;
      block_face.points[2].y = y_plane + 1.0f;
      block_face.points[2].z = z_row;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 0.0f;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 0.0f;
      // right side unshared vertex
      block_face.points[3].x = x_col + 1.0f;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row + 1.0f;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 0.0f;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_TOP;
      return block_face;
      
    case BLOCK_FACE_BOTTOM:
      block_face.points[0].x = x_col;
      block_face.points[0].y = y_plane;
      block_face.points[0].z = z_row;
      block_face.points[1].x = x_col + 1.0f;
      block_face.points[1].y = y_plane;
      block_face.points[1].z = z_row;
      block_face.points[2].x = x_col;
      block_face.points[2].y = y_plane;
      block_face.points[2].z = z_row + 1.0f;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 0.0f;
      block_face.uv[1].u = 1.0f;
      block_face.uv[1].v = 0.0f;
      block_face.uv[2].u = 0.0f;
      block_face.uv[2].v = 1.0f;
      // right side unshared vertex
      block_face.points[3].x = x_col + 1.0f;
      block_face.points[3].y = y_plane;
      block_face.points[3].z = z_row + 1.0f;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 0.0f;
      block_face.uv[4].v = 1.0f;
      block_face.uv[5].u = 1.0f;
      block_face.uv[5].v = 0.0f;
      block_face.block_face_type = BLOCK_FACE_BOTTOM;
    return block_face;
    
    case BLOCK_FACE_LEFT:
      block_face.points[0].x = x_col;
      block_face.points[0].y = y_plane;
      block_face.points[0].z = z_row + 1.0f;
      block_face.points[1].x = x_col;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row + 1.0f;
      block_face.points[2].x = x_col;
      block_face.points[2].y = y_plane;
      block_face.points[2].z = z_row;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 0.0f;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 0.0f;
      // right side unshared vertex
      block_face.points[3].x = x_col;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 0.0f;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_LEFT;
      return block_face;
      
    case BLOCK_FACE_RIGHT:
      block_face.points[0].x = x_col + 1.0f;
      block_face.points[0].y = y_plane;
      block_face.points[0].z = z_row;
      block_face.points[1].x = x_col + 1.0f;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row;
      block_face.points[2].x = x_col + 1.0f;
      block_face.points[2].y = y_plane;
      block_face.points[2].z = z_row + 1.0f;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 0.0f;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 0.0f;
      // right side unshared vertex
      block_face.points[3].x = x_col + 1.0f;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row + 1.0f;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 0.0f;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_RIGHT;
      return block_face;
      
    case BLOCK_FACE_BACK:
      block_face.points[0].x = x_col + 1.0f;
      block_face.points[0].y = y_plane;
      block_face.points[0].z = z_row + 1.0f;
      block_face.points[1].x = x_col + 1.0f;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row + 1.0f;
      block_face.points[2].x = x_col;
      block_face.points[2].y = y_plane;
      block_face.points[2].z = z_row + 1.0f;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 0.0f;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 0.0f;
      // right side unshared vertex
      block_face.points[3].x = x_col;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row + 1.0f;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 0.0f;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_BACK;
      return block_face;
      
    case BLOCK_FACE_FRONT:
    default:
      block_face.points[0].x = x_col;
      block_face.points[0].y = y_plane;
      block_face.points[0].z = z_row;
      block_face.points[1].x = x_col;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row;
      block_face.points[2].x = x_col + 1.0f;
      block_face.points[2].y = y_plane;
      block_face.points[2].z = z_row;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 0.0f;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 0.0f;
      // right side unshared vertex
      block_face.points[3].x = x_col + 1.0f;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 0.0f;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_FRONT;
      return block_face;
  }
}

inline block_face_t generate_fluid_top_block_face(block_face_direction_t block_face_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col){
  block_face_t block_face = {0};

  switch(block_face_type){
    case BLOCK_FACE_TOP:
    case BLOCK_FACE_BOTTOM:
    return block_face;
    
    case BLOCK_FACE_LEFT:
      block_face.points[0].x = x_col;
      block_face.points[0].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[0].z = z_row + 1.0f;
      block_face.points[1].x = x_col;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row + 1.0f;
      block_face.points[2].x = x_col;
      block_face.points[2].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[2].z = z_row;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      // right side unshared vertex
      block_face.points[3].x = x_col;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_LEFT;
      return block_face;
      
    case BLOCK_FACE_RIGHT:
      block_face.points[0].x = x_col + 1.0f;
      block_face.points[0].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[0].z = z_row;
      block_face.points[1].x = x_col + 1.0f;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row;
      block_face.points[2].x = x_col + 1.0f;
      block_face.points[2].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[2].z = z_row + 1.0f;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      // right side unshared vertex
      block_face.points[3].x = x_col + 1.0f;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row + 1.0f;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_RIGHT;
      return block_face;
      
    case BLOCK_FACE_BACK:
      block_face.points[0].x = x_col + 1.0f;
      block_face.points[0].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[0].z = z_row + 1.0f;
      block_face.points[1].x = x_col + 1.0f;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row + 1.0f;
      block_face.points[2].x = x_col;
      block_face.points[2].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[2].z = z_row + 1.0f;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      // right side unshared vertex
      block_face.points[3].x = x_col;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row + 1.0f;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_BACK;
      return block_face;
      
    case BLOCK_FACE_FRONT:
    default:
      block_face.points[0].x = x_col;
      block_face.points[0].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[0].z = z_row;
      block_face.points[1].x = x_col;
      block_face.points[1].y = y_plane + 1.0f;
      block_face.points[1].z = z_row;
      block_face.points[2].x = x_col + 1.0f;
      block_face.points[2].y = y_plane + 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.points[2].z = z_row;
      block_face.uv[0].u = 0.0f;
      block_face.uv[0].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[1].u = 0.0f;
      block_face.uv[1].v = 1.0f;
      block_face.uv[2].u = 1.0f;
      block_face.uv[2].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      // right side unshared vertex
      block_face.points[3].x = x_col + 1.0f;
      block_face.points[3].y = y_plane + 1.0f;
      block_face.points[3].z = z_row;
      block_face.uv[3].u = 1.0f;
      block_face.uv[3].v = 1.0f;
      block_face.uv[4].u = 1.0f;
      block_face.uv[4].v = 1.0f - BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      block_face.uv[5].u = 0.0f;
      block_face.uv[5].v = 1.0f;
      block_face.block_face_type = BLOCK_FACE_FRONT;
      return block_face;
  }
}

block_raycast_hit_t raycast_blocks(
  vector3_t ray_origin,
  vector3_t ray_direction,
  float max_distance
){
  block_raycast_hit_t hit_result = {0};

  int block_x = (int)floorf(ray_origin.x);
  int block_y = (int)floorf(ray_origin.y);
  int block_z = (int)floorf(ray_origin.z);

  int step_x = (ray_direction.x > 0.0f) ? 1 : -1;
  int step_y = (ray_direction.y > 0.0f) ? 1 : -1;
  int step_z = (ray_direction.z > 0.0f) ? 1 : -1;

  float next_boundary_x = (step_x > 0) ? (float)(block_x + 1) : (float)block_x;
  float next_boundary_y = (step_y > 0) ? (float)(block_y + 1) : (float)block_y;
  float next_boundary_z = (step_z > 0) ? (float)(block_z + 1) : (float)block_z;

  float t_max_x = (ray_direction.x != 0.0f) ? (next_boundary_x - ray_origin.x) / ray_direction.x : FLT_MAX;
  float t_max_y = (ray_direction.y != 0.0f) ? (next_boundary_y - ray_origin.y) / ray_direction.y : FLT_MAX;
  float t_max_z = (ray_direction.z != 0.0f) ? (next_boundary_z - ray_origin.z) / ray_direction.z : FLT_MAX;

  float t_delta_x = (ray_direction.x != 0.0f) ? fabsf(1.0f / ray_direction.x) : FLT_MAX;
  float t_delta_y = (ray_direction.y != 0.0f) ? fabsf(1.0f / ray_direction.y) : FLT_MAX;
  float t_delta_z = (ray_direction.z != 0.0f) ? fabsf(1.0f / ray_direction.z) : FLT_MAX;

  float current_distance = 0.0f;

  while(current_distance <= max_distance){
    if(block_x >= 0 && block_x < MAP_NUM_BLOCKS_X &&
       block_y >= 0 && block_y < MAP_NUM_BLOCKS_Y &&
       block_z >= 0 && block_z < MAP_NUM_BLOCKS_Z
    ){
      // only hit blocks that are non AIR and non FLUID
      if(block_at(block_y, block_z, block_x).type != BLOCK_TYPE_AIR &&
         block_materials[block_at(block_y, block_z, block_x).type].block_fluid_type != BLOCK_FLUID_WATER
    ){
        hit_result.hit = 1;
        hit_result.block_x = block_x;
        hit_result.block_y = block_y;
        hit_result.block_z = block_z;
        hit_result.distance = current_distance;
        return hit_result;
      }
    }

    if(t_max_x < t_max_y && t_max_x < t_max_z){
      block_x += step_x;
      current_distance = t_max_x;
      t_max_x += t_delta_x;

      hit_result.normal_x = -step_x;
      hit_result.normal_y = 0;
      hit_result.normal_z = 0;
    }
    else if(t_max_y < t_max_z){
      block_y += step_y;
      current_distance = t_max_y;
      t_max_y += t_delta_y;

      hit_result.normal_x = 0;
      hit_result.normal_y = -step_y;
      hit_result.normal_z = 0;
    }
    else{
      block_z += step_z;
      current_distance = t_max_z;
      t_max_z += t_delta_z;

      hit_result.normal_x = 0;
      hit_result.normal_y = 0;
      hit_result.normal_z = -step_z;
    }
  }

  return hit_result;
}

void assign_block_material(int32_t block_y, int32_t block_z, int32_t block_x, block_type_t block_type){

  if(block_y >= MAP_NUM_BLOCKS_Y || block_z >= MAP_NUM_BLOCKS_Z || block_x >= MAP_NUM_BLOCKS_X || block_y < 0 || block_z < 0 || block_x < 0) return;

  block_t *block = &block_at(block_y, block_z, block_x);

  block->type = block_type;
  block->current_mass = block_materials[block_type].mass;
  if(block_is_opaque(block)) block->sky_light = 0;
  if(block_is_emissive(block)) block->block_light = MAX_BLOCK_LIGHT;
  else block->block_light = 0;
}

char *get_block_material_name(block_type_t block_type){
  return block_materials[block_type].material_name;
}

void set_block_orientation(int32_t block_y, int32_t block_z, int32_t block_x, vector3_t player_position){
  vector2_t block_center = {block_x + 0.5f, block_z + 0.5f};

  vector2_t player_pos = {
    player_position.x - block_center.x,
    player_position.z - block_center.y
  };

  vector2_t block_v1 = {0.5f, -0.5f};
  vector2_t block_v2 = {0.5f, 0.5f};

  float player_dot_v1 = player_pos.x * block_v1.x + player_pos.y * block_v1.y;
  float player_dot_v2 = player_pos.x * block_v2.x + player_pos.y * block_v2.y;

  if(player_dot_v1 >= 0.0f && player_dot_v2 <= 0.0f)
    block_at(block_y, block_z, block_x).orientation = BLOCK_ORIENTATION_FRONT;
  else if(player_dot_v1 >= 0.0f && player_dot_v2 >= 0.0f)
    block_at(block_y, block_z, block_x).orientation = BLOCK_ORIENTATION_RIGHT;
  else if(player_dot_v1 <= 0.0f && player_dot_v2 >= 0.0f)
    block_at(block_y, block_z, block_x).orientation = BLOCK_ORIENTATION_BACK;
  else if(player_dot_v1 <= 0.0f && player_dot_v2 <= 0.0f)
    block_at(block_y, block_z, block_x).orientation = BLOCK_ORIENTATION_LEFT;
}

bool block_is_transparent(block_t *block){
  return 
    block->type == BLOCK_TYPE_AIR   || 
    block->type == BLOCK_TYPE_GLASS || 
    block_materials[block->type].block_fluid_type == BLOCK_FLUID_WATER;
}

bool block_is_opaque(block_t *block){
  return 
    block->type != BLOCK_TYPE_AIR   && 
    block->type != BLOCK_TYPE_GLASS && 
    block_materials[block->type].block_fluid_type != BLOCK_FLUID_WATER;
}

bool block_is_emissive(block_t *block){
  return block_materials[block->type].emissive_color.uint32 != 0x00000000;
}

bool block_is_stone(block_t *block){
  return 
    block->type == BLOCK_TYPE_VOLCANIC2 ||
    block->type == BLOCK_TYPE_VOLCANIC3 ||
    block->type == BLOCK_TYPE_STONE0 ||
    block->type == BLOCK_TYPE_STONE1 ||
    block->type == BLOCK_TYPE_STONE2 ||
    block->type == BLOCK_TYPE_STONE3 ||
    block->type == BLOCK_TYPE_STONE4 ||
    block->type == BLOCK_TYPE_STONE5 ||
    block->type == BLOCK_TYPE_STONE6 ||
    block->type == BLOCK_TYPE_STONE7 ||
    block->type == BLOCK_TYPE_STONE8;
}

bool block_is_tree_leaves(block_t *block){
  return 
    block->type == BLOCK_TYPE_LEAVES0 ||
    block->type == BLOCK_TYPE_LEAVES1 ||
    block->type == BLOCK_TYPE_LEAVES2 ||
    block->type == BLOCK_TYPE_LEAVES3 ||
    block->type == BLOCK_TYPE_LEAVES4 ||
    block->type == BLOCK_TYPE_LEAVES5 ||
    block->type == BLOCK_TYPE_LEAVES6 ||
    block->type == BLOCK_TYPE_LEAVES7 ||
    block->type == BLOCK_TYPE_LEAVES8 ||
    block->type == BLOCK_TYPE_LEAVES9 ||
    block->type == BLOCK_TYPE_CACTUS_FLOWER0;
}

bool block_is_tree_trunk(block_t *block){
  return
    block->type == BLOCK_TYPE_TREE_TRUNK0 ||
    block->type == BLOCK_TYPE_TREE_TRUNK1 ||
    block->type == BLOCK_TYPE_TREE_TRUNK2 ||
    block->type == BLOCK_TYPE_TREE_TRUNK3 ||
    block->type == BLOCK_TYPE_TREE_TRUNK4 ||
    block->type == BLOCK_TYPE_TREE_TRUNK5 ||
    block->type == BLOCK_TYPE_TREE_TRUNK6 ||
    block->type == BLOCK_TYPE_TREE_TRUNK7 ||
    block->type == BLOCK_TYPE_CACTUS_TRUNK0;
}

bool block_is_dirt(block_t *block){
  return
    block->type == BLOCK_TYPE_GRASSLANDS_DIRT ||
    block->type == BLOCK_TYPE_FOREST_DIRT ||
    block->type == BLOCK_TYPE_SAND0 ||
    block->type == BLOCK_TYPE_GRASSLANDS_GRASS ||
    block->type == BLOCK_TYPE_GRASSLANDS_CHERRY_GRASS ||
    block->type == BLOCK_TYPE_FOREST_GRASS ||
    block->type == BLOCK_TYPE_MOSS;
}

bool block_is_soil(block_t *block){
  return
    block_is_dirt(block) || block_is_sand(block);
}

bool block_is_sand(block_t *block){
  return
    block->type == BLOCK_TYPE_SAND0;
}

bool block_is_water(block_t *block){
  return 
    block->type == BLOCK_TYPE_WATER0 ||
    block->type == BLOCK_TYPE_BOG0;
}

bool block_is_lava(block_t *block){
  return 
    block->type == BLOCK_TYPE_LAVA0 ||
    block->type == BLOCK_TYPE_LAVA1;
}

void place_pond(int32_t z_row, int32_t x_col){
  uint32_t seed = (uint32_t)(z_row * 73856093u) ^ (uint32_t)(x_col * 19349663u); // two prime numbers for hashing
  bool will_spawn = random_spawn_from_seed(seed, 0.005f);

  if(!will_spawn) return;

  // find first solid block along y
  int32_t first_solid_y = 0;
  for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
    if(block_materials[block_at(y, z_row, x_col).type].solid){
      first_solid_y = y;
      break;
    }
  }

  uint32_t pond_size_seed = seed ^ 0xF12C77B3u;

  int32_t pond_size = 3.0f + (double)(6 - 3 + 1) * (double)(rand_i(pond_size_seed) / (double)UINT32_MAX);

  for(int32_t z = z_row; z < z_row + pond_size; z++){
    for(int32_t x = x_col; x < x_col + pond_size; x++){
      assign_block_material(first_solid_y, z, x, BLOCK_TYPE_WATER0);
    }
  }
}

bool should_render_face(block_face_direction_t face_direction, int32_t y, int32_t z, int32_t x){
  block_t *block = &block_at(y, z, x);

  switch(face_direction){
    case BLOCK_FACE_LEFT:
      if(x - 1 < 0) return false;
      block_t *block_left = &block_at(y, z, x - 1);
      if(block_materials[block->type].block_fluid_type != BLOCK_FLUID_NONE){
        if(block_left->type == BLOCK_TYPE_AIR) return true;
        return false;
      }
      else{
        if(block_left->type == BLOCK_TYPE_AIR || block_left->type == BLOCK_TYPE_GLASS) return true;
        if(block_materials[block_left->type].block_fluid_type != BLOCK_FLUID_NONE) return true;
        return false;
      }
      break;

    case BLOCK_FACE_RIGHT:
      if(x + 1 >= MAP_NUM_BLOCKS_X) return false;
      block_t *block_right = &block_at(y, z, x + 1);
      if(block_materials[block->type].block_fluid_type != BLOCK_FLUID_NONE){
        if(block_right->type == BLOCK_TYPE_AIR) return true;

        return false;
      }
      else{
        if(block_right->type == BLOCK_TYPE_AIR || block_right->type == BLOCK_TYPE_GLASS) return true;
        if(block_materials[block_right->type].block_fluid_type != BLOCK_FLUID_NONE) return true;
        return false;
      }
      break;

    case BLOCK_FACE_TOP:
      if(y + 1 >= MAP_NUM_BLOCKS_Y) return false;
      if(block_materials[block->type].block_fluid_type != BLOCK_FLUID_NONE){
        return block_is_surface_fluid(y, z, x);
      }
      else{
        if(block_at(y + 1, z, x).type == BLOCK_TYPE_AIR || block_at(y + 1, z, x).type == BLOCK_TYPE_GLASS || block_at(y + 1, z, x).type == BLOCK_TYPE_WATER0) 
          return true;

        return false;
      }
      break;

    case BLOCK_FACE_BOTTOM:
      if(y - 1 < 0) return false;
      block_t *block_bottom = &block_at(y - 1, z, x);
      if(block_materials[block->type].block_fluid_type != BLOCK_FLUID_NONE){
        if(block_bottom->type == BLOCK_TYPE_AIR) return true;
        return false;
      }
      else{
        if(block_bottom->type == BLOCK_TYPE_AIR || block_bottom->type == BLOCK_TYPE_GLASS) return true;
        if(block_materials[block_bottom->type].block_fluid_type != BLOCK_FLUID_NONE) return true;
        return false;
      }
      break;

    case BLOCK_FACE_FRONT:
      if(z - 1 < 0) return false;
      block_t *block_front = &block_at(y, z - 1, x);
      if(block_materials[block->type].block_fluid_type != BLOCK_FLUID_NONE){
        if(block_front->type == BLOCK_TYPE_AIR) return true;
        return false;
      }
      else{
        if(block_front->type == BLOCK_TYPE_AIR || block_front->type == BLOCK_TYPE_GLASS) return true;
        if(block_materials[block_front->type].block_fluid_type != BLOCK_FLUID_NONE) return true;
        return false;
      }
      break;

    case BLOCK_FACE_BACK:
    default:
      if(z + 1 >= MAP_NUM_BLOCKS_Z) return false;
      block_t *block_back = &block_at(y, z + 1, x);
      if(block_materials[block->type].block_fluid_type != BLOCK_FLUID_NONE){
        if(block_back->type == BLOCK_TYPE_AIR) return true;
        return false;
      }
      else{
        if(block_back->type == BLOCK_TYPE_AIR || block_back->type == BLOCK_TYPE_GLASS) return true;
        if(block_materials[block_back->type].block_fluid_type != BLOCK_FLUID_NONE) return true;
        return false;
      }
      break;
  }
}

// a block is a top water block if:
//   block is water and block on top is air
//   a block is water and at least one of its XZ neighbours is a top water block
bool block_is_surface_fluid(int32_t y, int32_t z, int32_t x){
  if(block_materials[block_at(y, z, x).type].block_fluid_type == BLOCK_FLUID_NONE) return false;

  if(y + 1 >= MAP_NUM_BLOCKS_Y || x + 1 >= MAP_NUM_BLOCKS_X || z + 1 >= MAP_NUM_BLOCKS_Z || x - 1 < 0 || z - 1 < 0) return false;

  // if block on top is fluid, return false
  if(block_materials[block_at(y + 1, z, x).type].block_fluid_type != BLOCK_FLUID_NONE) return false;

  if(block_at(y + 1, z, x).type == BLOCK_TYPE_AIR) return true;

  if(
    (block_materials[block_at(y, z - 1,     x).type].block_fluid_type != BLOCK_FLUID_NONE && block_at(y + 1, z - 1,     x).type == BLOCK_TYPE_AIR) ||
    (block_materials[block_at(y, z + 1,     x).type].block_fluid_type != BLOCK_FLUID_NONE && block_at(y + 1, z + 1,     x).type == BLOCK_TYPE_AIR) ||
    (block_materials[block_at(y, z    , x - 1).type].block_fluid_type != BLOCK_FLUID_NONE && block_at(y + 1, z    , x - 1).type == BLOCK_TYPE_AIR) ||
    (block_materials[block_at(y, z    , x + 1).type].block_fluid_type != BLOCK_FLUID_NONE && block_at(y + 1, z    , x + 1).type == BLOCK_TYPE_AIR)
  ) return true;

  return false;
}

// connector is rendered if:
//   block is top fluid surface and upper neighbours are fluid too
//   OR
//   block is top fluid surface and same level neighbours are fluid too but not top surfaces
bool should_render_fluid_block_connectors(block_face_direction_t face_direction, int32_t y, int32_t z, int32_t x){
  
  if(!block_is_surface_fluid(y, z, x)) return false;

  switch(face_direction){
    case BLOCK_FACE_LEFT:
      if(
        (block_materials[block_at(y + 1, z, x - 1).type].block_fluid_type != BLOCK_FLUID_NONE && block_materials[block_at(y, z, x - 1).type].block_fluid_type != BLOCK_FLUID_NONE) ||
         (block_materials[block_at(y, z, x - 1).type].block_fluid_type != BLOCK_FLUID_NONE && !block_is_surface_fluid(y, z, x - 1))

      ) return true;
      return false;
      
    case BLOCK_FACE_RIGHT:
      if((block_materials[block_at(y + 1, z, x + 1).type].block_fluid_type != BLOCK_FLUID_NONE && block_materials[block_at(y, z, x + 1).type].block_fluid_type != BLOCK_FLUID_NONE) ||
         (block_materials[block_at(y, z, x + 1).type].block_fluid_type != BLOCK_FLUID_NONE && !block_is_surface_fluid(y, z, x + 1))
    ) return true;
      return false;

    case BLOCK_FACE_BACK:
      if((block_materials[block_at(y + 1, z + 1, x).type].block_fluid_type != BLOCK_FLUID_NONE && block_materials[block_at(y, z + 1, x).type].block_fluid_type != BLOCK_FLUID_NONE) ||
         (block_materials[block_at(y, z + 1, x).type].block_fluid_type != BLOCK_FLUID_NONE && !block_is_surface_fluid(y, z + 1, x))
    ) return true;
      return false;

    case BLOCK_FACE_FRONT:
      if((block_materials[block_at(y + 1, z - 1, x).type].block_fluid_type != BLOCK_FLUID_NONE && block_materials[block_at(y, z - 1, x).type].block_fluid_type != BLOCK_FLUID_NONE) ||
         (block_materials[block_at(y, z - 1, x).type].block_fluid_type != BLOCK_FLUID_NONE && !block_is_surface_fluid(y, z - 1, x))
    ) return true;
      return false;

    default:
      return false;
  }

  return false;
}

bool should_shift_block_faces(int32_t y_plane, int32_t z_row, int32_t x_col){

  if(block_materials[block_at(y_plane, z_row, x_col).type].block_fluid_type != BLOCK_FLUID_NONE){
    return block_is_surface_fluid(y_plane, z_row, x_col);
  }

  return false;
}

// for blocks that are a fluid top surface, we can't sample the light from the block right above the surface
// because it could be a solid block, and solid blocks have 0 light
// so we test the light around the surface, to find the largest light amount
light_sample_t get_fluid_surface_light(int32_t y, int32_t z, int32_t x){
  light_sample_t light = {
    .sky_light = block_at(y, z, x).sky_light,
    .block_light = block_at(y, z, x).block_light
  };

  if(y + 1 < MAP_NUM_BLOCKS_Y && !block_is_opaque(&block_at(y + 1, z, x))){
    if(block_at(y + 1, z, x).sky_light > light.sky_light)
      light.sky_light = block_at(y + 1, z, x).sky_light;

    if(block_at(y + 1, z, x).block_light > light.block_light)
      light.block_light = block_at(y + 1, z, x).block_light;
  }

  if(z - 1 >= 0){
    if(block_at(y, z - 1, x).sky_light > light.sky_light)
      light.sky_light = block_at(y, z - 1, x).sky_light;

    if(block_at(y, z - 1, x).block_light > light.block_light)
      light.block_light = block_at(y, z - 1, x).block_light;
  }

  if(z + 1 < MAP_NUM_BLOCKS_Z){
    if(block_at(y, z + 1, x).sky_light > light.sky_light)
      light.sky_light = block_at(y, z + 1, x).sky_light;

    if(block_at(y, z + 1, x).block_light > light.block_light)
      light.block_light = block_at(y, z + 1, x).block_light;
  }

  if(x - 1 >= 0){
    if(block_at(y, z, x - 1).sky_light > light.sky_light)
      light.sky_light = block_at(y, z, x - 1).sky_light;

    if(block_at(y, z, x - 1).block_light > light.block_light)
      light.block_light = block_at(y, z, x - 1).block_light;
  }

  if(x + 1 < MAP_NUM_BLOCKS_X){
    if(block_at(y, z, x + 1).sky_light > light.sky_light)
      light.sky_light = block_at(y, z, x + 1).sky_light;

    if(block_at(y, z, x + 1).block_light > light.block_light)
      light.block_light = block_at(y, z, x + 1).block_light;
  }

  return light;
}

float evaluate_spline(vector2_t *spline_points, int32_t num_spline_points, float x){
  if(x <= spline_points[0].x)
    return spline_points[0].y;

  if(x >= spline_points[num_spline_points - 1].x)
    return spline_points[num_spline_points - 1].y;

  for(int32_t spline_point_index = 0; spline_point_index < num_spline_points - 1; spline_point_index++){
    vector2_t left_spline_point  = spline_points[spline_point_index];
    vector2_t right_spline_point = spline_points[spline_point_index + 1];

    if(x >= left_spline_point.x && x <= right_spline_point.x){
      float t = (x - left_spline_point.x) / (right_spline_point.x - left_spline_point.x);

      return lerp(t, left_spline_point.y, right_spline_point.y);
    }
  }

  return spline_points[num_spline_points - 1].y;
}

float evaluate_peaks_and_valleys_spline(float peaks_and_valleys_noise_value){
  vector2_t peaks_and_valleys_spline[] = {
    {-1.00f,  -10.0f},
    {-0.60f,  -60.0f},
    {-0.20f,   0.0f},
    { 0.25f,  10.0f},
    { 0.55f,  60.0f},
    { 0.80f,  160.0f},
    { 1.00f,  60.0f}
  };

  return evaluate_spline(
    peaks_and_valleys_spline,
    sizeof(peaks_and_valleys_spline) / sizeof(peaks_and_valleys_spline[0]),
    peaks_and_valleys_noise_value
  );
}

float evaluate_height_spline(float height_noise_value, float peaks_and_valleys_noise_value){
  float peaks_and_valleys_height = evaluate_peaks_and_valleys_spline(peaks_and_valleys_noise_value);

  vector2_t height_spline[] = {
    {-1.00f, -30.0f},
    {-0.60f, -15.0f},
    {-0.20f,   0.0f},
    { 0.25f,   10.0f},
    // This point is nested. Instead of fixed height 20/80/etc, it uses the peaks_and_valleys spline result.
    { 0.35f, 20.0f},
    //{ 0.50f, 33.0f},
    { 0.50f, peaks_and_valleys_height},
    { 1.00f,  40.0f}
  };

  return evaluate_spline(
    height_spline,
    sizeof(height_spline) / sizeof(height_spline[0]),
    height_noise_value
  );
}

float find_perlin_noise_slice_absolute_max(float frequency, float y){
  float absolute_max = 0.0f;

  for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
    for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
      float noise = perlin_noise_3d((float)x * frequency, y, (float)z * frequency);
      float absolute_noise = fabsf(noise);

      if(absolute_noise > absolute_max){
        absolute_max = absolute_noise;
      }
    }
  }

  if(absolute_max < 0.0001f){
    absolute_max = 1.0f;
  }

  return absolute_max;
}