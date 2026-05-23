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
#include "log.h"

// Y slowest changing dimension, X is fastest changing
block_t *blocks;

// block currently being looked at
block_raycast_hit_t looked_at_block;

block_material_t *block_materials;

void init_block_materials(){
  // AIR
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "air",
      .mining_amount        = 0,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = false,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = NULL,
      .back_texture       = NULL,
      .left_texture       = NULL,
      .right_texture      = NULL,
      .top_texture        = NULL,
      .bottom_texture     = NULL
    })  
  );

  // LAVA
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "lava0",
      .mining_amount      = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .opacity            = 1.0f,
      .reflects_light     = false,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/lava0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/lava0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/lava0.png"),
      .right_texture      = load_texture("./assets/textures/blocks/lava0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/lava0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/lava0.png")
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "lava1",
      .mining_amount      = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .opacity            = 1.0f,
      .reflects_light     = false,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/lava1.png"),
      .back_texture       = load_texture("./assets/textures/blocks/lava1.png"),
      .left_texture       = load_texture("./assets/textures/blocks/lava1.png"),
      .right_texture       = load_texture("./assets/textures/blocks/lava1.png"),
      .top_texture        = load_texture("./assets/textures/blocks/lava1.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/lava1.png")
    })  
  );

  // VOLCANIC
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .opacity            = 1.0f,
      .reflects_light     = false,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/volcanic0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/volcanic0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/volcanic0.png"),
      .right_texture       = load_texture("./assets/textures/blocks/volcanic0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/volcanic0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/volcanic0.png")
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic1",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF80FFFF},
      .opacity            = 1.0f,
      .reflects_light     = false,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/volcanic1.png"),
      .back_texture       = load_texture("./assets/textures/blocks/volcanic1.png"),
      .left_texture       = load_texture("./assets/textures/blocks/volcanic1.png"),
      .right_texture       = load_texture("./assets/textures/blocks/volcanic1.png"),
      .top_texture        = load_texture("./assets/textures/blocks/volcanic1.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/volcanic1.png")
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic2",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0xFF20FFFF},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/volcanic2.png"),
      .back_texture       = load_texture("./assets/textures/blocks/volcanic2.png"),
      .left_texture       = load_texture("./assets/textures/blocks/volcanic2.png"),
      .right_texture       = load_texture("./assets/textures/blocks/volcanic2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/volcanic2.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/volcanic2.png")
    })  
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "volcanic3",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0x00000000},
      .emissive_color     = (color_t){.uint32 = 0000000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/volcanic3.png"),
      .back_texture       = load_texture("./assets/textures/blocks/volcanic3.png"),
      .left_texture       = load_texture("./assets/textures/blocks/volcanic3.png"),
      .right_texture       = load_texture("./assets/textures/blocks/volcanic3.png"),
      .top_texture        = load_texture("./assets/textures/blocks/volcanic3.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/volcanic3.png")
    })  
  );

  // STONE
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone0.png"),
      .right_texture      = load_texture("./assets/textures/blocks/stone0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone0.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone1",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone1.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone1.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone1.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone1.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone1.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone1.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone2",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone2.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone2.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone2.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone2.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone2.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone3",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone3.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone3.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone3.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone3.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone3.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone3.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone4",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone4.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone4.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone4.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone4.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone4.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone4.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone5",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone5.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone5.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone5.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone5.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone5.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone5.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone6",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone6.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone6.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone6.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone6.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone6.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone6.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone7",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone7.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone7.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone7.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone7.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone7.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone7.png")
    })
  );
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "stone8",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/stone8.png"),
      .back_texture       = load_texture("./assets/textures/blocks/stone8.png"),
      .left_texture       = load_texture("./assets/textures/blocks/stone8.png"),
      .right_texture       = load_texture("./assets/textures/blocks/stone8.png"),
      .top_texture        = load_texture("./assets/textures/blocks/stone8.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/stone8.png")
    })
  );

  // COAL
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "coal",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/coal_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/coal_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/coal_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/coal_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/coal_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/coal_top.png")
    })
  );

  // IRON
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "iron",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/iron_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/iron_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/iron_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/iron_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/iron_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/iron_top.png")
    })
  );

  // DIAMOND
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "diamond",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/diamond_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/diamond_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/diamond_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/diamond_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/diamond_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/diamond_top.png")
    })
  );

  // SILVER
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "silver",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/silver_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/silver_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/silver_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/silver_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/silver_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/silver_top.png")
    })
  );

  // GOLD
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "gold",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/gold_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/gold_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/gold_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/gold_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/gold_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/gold_top.png")
    })
  );

  // DIRT
  array_push(
    block_materials, 
    ((block_material_t){
      .material_name      = "dirt",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF408080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/dirt_2.png"),
      .back_texture       = load_texture("./assets/textures/blocks/dirt_2.png"),
      .left_texture       = load_texture("./assets/textures/blocks/dirt_2.png"),
      .right_texture       = load_texture("./assets/textures/blocks/dirt_2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/dirt_2.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/dirt_2.png")
    })
  );

  // GRASS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "grass",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF0FF000},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/grass_side_2.png"),
      .back_texture       = load_texture("./assets/textures/blocks/grass_side_2.png"),
      .left_texture       = load_texture("./assets/textures/blocks/grass_side_2.png"),
      .right_texture       = load_texture("./assets/textures/blocks/grass_side_2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/grass_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/grass_top.png")
    })
  );

  // TREE TRUNKS
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk0_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk0_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk0_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk0_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk0_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk0_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk1",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk1_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk1_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk1_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk1_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk1_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk1_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk2",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk2_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk2_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk2_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk2_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk2_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk2_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk3",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk3_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk3_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk3_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk3_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk3_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk3_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk4",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk4_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk4_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk4_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk4_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk4_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk4_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk5",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk5_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk5_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk5_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk5_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk5_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk5_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk6",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk6_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk6_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk6_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk6_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk6_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk6_top.png")
    }
  );
  array_push(
    block_materials,
    (block_material_t){
      .material_name      = "treetrunk7",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF8080FF},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/treetrunk7_side.png"),
      .back_texture       = load_texture("./assets/textures/blocks/treetrunk7_side.png"),
      .left_texture       = load_texture("./assets/textures/blocks/treetrunk7_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/treetrunk7_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/treetrunk7_top.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/treetrunk7_top.png")
    }
  );

  // LEAVES
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves0.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves0.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves1",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves1.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves1.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves1.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves1.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves1.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves1.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves2",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves2.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves2.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves2.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves2.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves2.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves2.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves3",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves3.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves3.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves3.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves3.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves3.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves3.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves4",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves4.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves4.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves4.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves4.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves4.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves4.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves5",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves5.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves5.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves5.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves5.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves5.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves5.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves6",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves6.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves6.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves6.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves6.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves6.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves6.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves7",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves7.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves7.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves7.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves7.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves7.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves7.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves8",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves8.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves8.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves8.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves8.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves8.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves8.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "leaves9",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/leaves9.png"),
      .back_texture       = load_texture("./assets/textures/blocks/leaves9.png"),
      .left_texture       = load_texture("./assets/textures/blocks/leaves9.png"),
      .right_texture       = load_texture("./assets/textures/blocks/leaves9.png"),
      .top_texture        = load_texture("./assets/textures/blocks/leaves9.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/leaves9.png")
    })
  );

  // FLOWER LEAVES
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "flowerleaves0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/flower_leaves0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/flower_leaves0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/flower_leaves0.png"),
      .right_texture      = load_texture("./assets/textures/blocks/flower_leaves0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/flower_leaves0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/flower_leaves0.png")
    })
  );
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "flowerleaves1",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF80FF20},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/flower_leaves1.png"),
      .back_texture       = load_texture("./assets/textures/blocks/flower_leaves1.png"),
      .left_texture       = load_texture("./assets/textures/blocks/flower_leaves1.png"),
      .right_texture      = load_texture("./assets/textures/blocks/flower_leaves1.png"),
      .top_texture        = load_texture("./assets/textures/blocks/flower_leaves1.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/flower_leaves1.png")
    })
  );

  // GLASS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "glass",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_ALPHA_TEST,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/glass0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/glass0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/glass0.png"),
      .right_texture       = load_texture("./assets/textures/blocks/glass0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/glass0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/glass0.png")
    })
  );

  // LAMPS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "lamp0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x0040FFFF},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/lamp0.png"),
      .back_texture       = load_texture("./assets/textures/blocks/lamp0.png"),
      .left_texture       = load_texture("./assets/textures/blocks/lamp0.png"),
      .right_texture       = load_texture("./assets/textures/blocks/lamp0.png"),
      .top_texture        = load_texture("./assets/textures/blocks/lamp0.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/lamp0.png")
    })
  );

  // OVENS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "oven0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/oven_front.png"),
      .back_texture       = load_texture("./assets/textures/blocks/oven.png"),
      .left_texture       = load_texture("./assets/textures/blocks/oven_side.png"),
      .right_texture       = load_texture("./assets/textures/blocks/oven_side.png"),
      .top_texture        = load_texture("./assets/textures/blocks/oven.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/oven.png")
    })
  );

  // CHESTS
  array_push(
    block_materials,
    ((block_material_t){
      .material_name      = "chest0",
      .mining_amount        = 10,
      .backface_culling   = true,
      .base_color         = (color_t){.uint32 = 0xFF808080},
      .emissive_color     = (color_t){.uint32 = 0x00000000},
      .opacity            = 1.0f,
      .reflects_light     = true,
      .dither             = false,
      .blend_mode         = BLEND_MODE_OPAQUE,
      .alpha_cutoff       = DEFAULT_ALPHA_CUTOFF,
      .front_texture      = load_texture("./assets/textures/blocks/chest_front.png"),
      .back_texture       = load_texture("./assets/textures/blocks/chest_sides.png"),
      .left_texture       = load_texture("./assets/textures/blocks/chest_sides.png"),
      .right_texture       = load_texture("./assets/textures/blocks/chest_sides.png"),
      .top_texture        = load_texture("./assets/textures/blocks/chest_top_bottom.png"),
      .bottom_texture     = load_texture("./assets/textures/blocks/chest_top_bottom.png")
    })
  );
}

void init_block_map(){

  blocks = malloc(sizeof(block_t) * MAP_NUM_VOXELS_Y * MAP_NUM_VOXELS_Z * MAP_NUM_VOXELS_X);
  if(blocks == NULL){
    print_log("ERROR: unable to allocate map memory");
    exit(1);
  }

  for(uint32_t y = 0; y < MAP_NUM_VOXELS_Y; y++){
    for(uint32_t z = 0; z < MAP_NUM_VOXELS_Z; z++){
      for(uint32_t x = 0; x < MAP_NUM_VOXELS_X; x++){
        block_at(y, z, x).solid = false;
        block_at(y, z, x).mineable = true;
        assign_block_material(y, z, x, BLOCK_TYPE_AIR);
        block_at(y, z, x).temperature = 30.0f;
        block_at(y, z, x).orientation = BLOCK_ORIENTATION_FRONT;

        if(y == (uint32_t)(0.4 * MAP_NUM_VOXELS_Y)){
          if(xorshift32() % 5 == 0){
            uint32_t random = xorshift32();
            if(random % 2 == 0){
              assign_block_material(y, z, x, BLOCK_TYPE_GRASS);
            }
            else{
              if(xorshift32() % 2 == 0)
                assign_block_material(y, z, x, BLOCK_TYPE_LEAVES0);
              else
                assign_block_material(y, z, x, BLOCK_TYPE_FLOWER_LEAVES0);
            }
            block_at(y, z, x).mineable = true;
            block_at(y, z, x).solid = true;
          }
        }
        if(y == (uint32_t)(0.4 * MAP_NUM_VOXELS_Y) - 1){
          if(xorshift32() % 5 == 0){
            assign_block_material(y, z, x, BLOCK_TYPE_DIRT);
          }
          else{
            assign_block_material(y, z, x, BLOCK_TYPE_GRASS);
          }
          block_at(y, z, x).mineable = true;
          block_at(y, z, x).solid = true;
        }
        if(y < (uint32_t)(0.4 * MAP_NUM_VOXELS_Y) - 1){
          assign_block_material(y, z, x, BLOCK_TYPE_DIRT);
          block_at(y, z, x).solid = true;
          block_at(y, z, x).mineable = true;
        }
        if(y <= (uint32_t)(0.3 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_STONE3);
          block_at(y, z, x).solid = true;
          block_at(y, z, x).mineable = true;
        }
        if(y <= (uint32_t)(0.2 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_STONE0);
          block_at(y, z, x).solid = true;
          block_at(y, z, x).mineable = true;
        }

        if((x <= 10 || x >= MAP_NUM_VOXELS_X - 10 || z <= 10 || z >= MAP_NUM_VOXELS_Z - 10) && y >= 0.4 * MAP_NUM_VOXELS_Y && y <= 0.43 * MAP_NUM_VOXELS_Y){
          if(xorshift32() % 5 == 0){
            assign_block_material(y, z, x, BLOCK_TYPE_LEAVES0);
            block_at(y, z, x).solid = true;
          }
          else{
            block_at(y, z, x).type = BLOCK_TYPE_AIR;
          }
        }

        // VOLCANIC
        if(y <= (uint32_t)(0.1 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_VOLCANIC3);
          block_at(y, z, x).mineable = true;
          block_at(y, z, x).solid = true;
          block_at(y, z, x).temperature = 100.0f;
        }
        if(y <= (uint32_t)(0.06 * MAP_NUM_VOXELS_Y)){
          if(xorshift32() % 3 == 0)
            assign_block_material(y, z, x, BLOCK_TYPE_VOLCANIC1);
          else
            assign_block_material(y, z, x, BLOCK_TYPE_VOLCANIC2);
          block_at(y, z, x).mineable = true;
          block_at(y, z, x).solid = true;
          block_at(y, z, x).temperature = 300.0f;
        }
        if(y <= (uint32_t)(0.04 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_VOLCANIC0);
          block_at(y, z, x).mineable = true;
          block_at(y, z, x).solid = true;
          block_at(y, z, x).temperature = 400.0f;
        }
        if(y <= (uint32_t)(0.02 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_LAVA1);
          block_at(y, z, x).mineable = true;
          block_at(y, z, x).solid = true;
          block_at(y, z, x).temperature = 400.0f;
        }
        if(y <= (uint32_t)(0.01 * MAP_NUM_VOXELS_Y)){
          assign_block_material(y, z, x, BLOCK_TYPE_LAVA0);
          block_at(y, z, x).mineable = true;
          block_at(y, z, x).solid = true;
          block_at(y, z, x).temperature = 800.0f;
        }
        if(y == 0){
          block_at(y, z, x).mineable = false;
          block_at(y, z, x).solid = true;
          block_at(y, z, x).temperature = 1800.0f;
        }

        block_at(y, z, x).aabb.x_min = x;
        block_at(y, z, x).aabb.x_max = x + 1.0;
        block_at(y, z, x).aabb.z_min = z;
        block_at(y, z, x).aabb.z_max = z + 1.0;
        block_at(y, z, x).aabb.y_min = y;
        block_at(y, z, x).aabb.y_max = y + 1.0;
      }
    }
  }

  place_trees();
  place_ores();
}

void get_block_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z){
  *start_x = clamp_int((int)floorf(aabb->x_min), 0, MAP_NUM_VOXELS_X - 1);
  *end_x   = clamp_int((int)floorf(aabb->x_max), 0, MAP_NUM_VOXELS_X - 1);

  *start_y = clamp_int((int)floorf(aabb->y_min), 0, MAP_NUM_VOXELS_Y - 1);
  *end_y   = clamp_int((int)floorf(aabb->y_max), 0, MAP_NUM_VOXELS_Y - 1);

  *start_z = clamp_int((int)floorf(aabb->z_min), 0, MAP_NUM_VOXELS_Z - 1);
  *end_z   = clamp_int((int)floorf(aabb->z_max), 0, MAP_NUM_VOXELS_Z - 1);
}

inline block_quad_t generate_block_quad(block_quad_type_t quad_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col){
  block_quad_t quad;

  switch(quad_type){
    case VOXEL_QUAD_TOP:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane + 1.0f;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row + 1.0f;
      quad.points[2].x = x_col + 1.0f;
      quad.points[2].y = y_plane + 1.0f;
      quad.points[2].z = z_row;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_TOP;
      return quad;
      
    case VOXEL_QUAD_BOTTOM:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col + 1.0f;
      quad.points[1].y = y_plane;
      quad.points[1].z = z_row;
      quad.points[2].x = x_col;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row + 1.0f;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 1.0f;
      quad.uv[1].v = 0.0f;
      quad.uv[2].u = 0.0f;
      quad.uv[2].v = 1.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 0.0f;
      quad.uv[4].v = 1.0f;
      quad.uv[5].u = 1.0f;
      quad.uv[5].v = 0.0f;
      quad.quad_type = VOXEL_QUAD_BOTTOM;
    return quad;
    
    case VOXEL_QUAD_LEFT:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row + 1.0f;
      quad.points[1].x = x_col;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row + 1.0f;
      quad.points[2].x = x_col;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_LEFT;
      return quad;
      
    case VOXEL_QUAD_RIGHT:
      quad.points[0].x = x_col + 1.0f;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col + 1.0f;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row;
      quad.points[2].x = x_col + 1.0f;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row + 1.0f;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_RIGHT;
      return quad;
      
    case VOXEL_QUAD_BACK:
      quad.points[0].x = x_col + 1.0f;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row + 1.0f;
      quad.points[1].x = x_col + 1.0f;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row + 1.0f;
      quad.points[2].x = x_col;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row + 1.0f;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row + 1.0f;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_BACK;
      return quad;
      
    case VOXEL_QUAD_FRONT:
    default:
      quad.points[0].x = x_col;
      quad.points[0].y = y_plane;
      quad.points[0].z = z_row;
      quad.points[1].x = x_col;
      quad.points[1].y = y_plane + 1.0f;
      quad.points[1].z = z_row;
      quad.points[2].x = x_col + 1.0f;
      quad.points[2].y = y_plane;
      quad.points[2].z = z_row;
      quad.uv[0].u = 0.0f;
      quad.uv[0].v = 0.0f;
      quad.uv[1].u = 0.0f;
      quad.uv[1].v = 1.0f;
      quad.uv[2].u = 1.0f;
      quad.uv[2].v = 0.0f;
      // right side unshared vertex
      quad.points[3].x = x_col + 1.0f;
      quad.points[3].y = y_plane + 1.0f;
      quad.points[3].z = z_row;
      quad.uv[3].u = 1.0f;
      quad.uv[3].v = 1.0f;
      quad.uv[4].u = 1.0f;
      quad.uv[4].v = 0.0f;
      quad.uv[5].u = 0.0f;
      quad.uv[5].v = 1.0f;
      quad.quad_type = VOXEL_QUAD_FRONT;
      return quad;
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
    if(block_x >= 0 && block_x < MAP_NUM_VOXELS_X &&
       block_y >= 0 && block_y < MAP_NUM_VOXELS_Y &&
       block_z >= 0 && block_z < MAP_NUM_VOXELS_Z
    ){
      if(block_at(block_y, block_z, block_x).solid){
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

void calculate_currently_looked_at_block(){
  looked_at_block = raycast_blocks(
    camera.position,
    camera.direction,
    MAX_VOXEL_LOOK_AT_DISTANCE
  );
}

void assign_block_material(int32_t block_y, int32_t block_z, int32_t block_x, block_type_t block_type){
  block_at(block_y, block_z, block_x).type = block_type;
  block_at(block_y, block_z, block_x).block_material = &block_materials[block_type];
  block_at(block_y, block_z, block_x).mining_amount = block_at(block_y, block_z, block_x).block_material->mining_amount;
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