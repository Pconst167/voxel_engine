/*
  3D Software Renderer
  Pipeline:
    model space
    world space + view space transform - scaling, rotation, translation
    view space  - all vertices translated and rotated so that camera is at origin
    backface culling
    frustrum clipping
    projection / perspective divide
    triangle rasterization

    DONE:

      mesh ordering front to back for helping with zbuffer
      then later bilinear filtering on textures for far away triangles

      for some objects we dont want the light to shine on just one side (when both camera and normal are on same side), such as trees,
      because then it looks wrong. so need to add a parameter for "two-sided-lighting"

      check for meshes totally outside frustrum and cull

      textures should be bound to objects not meshes

      for each chunk, for each object, mark a face as fully inside chunk, or not fully inside so we can skip clipping
      OR: still only TEST for clipping, but dont clip directly. if triangle crosses frustrum boundary then clip.
      then for fully inside, we only test for clipping, but for intersecting chunks, clip all triangles directly.

    TODO:
      biomes
      lava/volcano
      stalactites
      floating isles
      fix player position when inside water and jumping upwards into air
      * maybe world can be infinite but in another way: with portals.
      could we make rivers of water and lava and have the textures scroll?

      change walking sound engine to small footstep sounds etc.
      add timers. 

      CONVERT BLOCK COORDINATES ETC TO -X TO X, ETC. I.E CENTERED AROUND ORIGIN. THIS IS BETTER FOR FLOATING PRECISION
      SO INSTEAD OF HAVING THE WORLD ORIGIN CORNER AT 0, WE PLACE THE WORLD CENTER AT 0 INSTEAD

      ADD MIPMAPS TO HELP WITH FAR AWAY SHIMMERING 

      add funcs to remove collision aabb from object, and change them

      add functions to add cube, sphere, cylinder, etc.

      convert other raster functions to fixed point

      for options such as adding culling etc, for objects or anything, use bit fields instead of bools, so I can see what the option is when looking
      at the function

      the function sequence at main, such as update state, move player, render etc, need to be in the correct order
      check if order is OK, and/or fix if necessary


      about lighting: we can interpolate the 3d x y z coordinates in a triangle per pixel, and use that to calculate the per pixel light values.
      for point light we can interpolate the real per pixel brightness with 1/r^2 law.
      but probably expensive.


      IMPLEMENT OCCLUSION CULLING: for each object, project its bounding box AABB 8 vertices to world_view->space->screen and from the 8 vertices form rectangle with min/max of x/y
      then form a grid inside rectangle. for each grid point, we replace its own depth with the nearest object depth, and test that vs zbuffer.
      only if all points are farther than zbuffer values, skip/cull object completely.
      when forming rectangle, clamp it against screen edges if it goes over.
      NOTE: after transforming to world_view, then find the nearest depth of BBOX. AFTER that we project to screen to form rectangle.
      but to find nearest depth, do not project yet.

      apply LOD? backface cull trees etc when avg_z > some distance

      add color buffer rendering function pointers so different functions can be used/selected

      perhaps create a parameter for absolute sun intensity, and then have the sky and directional light track its brightness too?
      dont forget that as the sun sets the sky needs to become bluer, and as the sun rises sky becomes less blue. (this will be done with a tint/color parameter for the self luminosity)

      add weather patterns so every day can be a new weather possibly, raining, snowing, sun etc.
      also each day can have different sky tints like greenish, yellowish, etc!
      perhaps also can use perlin noise to apply tints to sky at places?

      also, sky should be darker on the side opposite sunset side. and perhaps have a greenish blue horizon band stronger there.
      patern: mid day: light green or blue horizon bands on all sides
              before/after sunrise/sunset: yellow to green to blue band
              at sunset: orange to yellow to green band that dissolves into blue sky upwards
      

      add stars to night sky:
        either a second skydome that slowly blends with the old one, while old disappears and is disabled
        or perhaps create it procedurally with hashes etc. and have the star pixels go from faint to solid white slowly 

      code for creating terrain meshes procedurally

      caves would be nice

    NOTES:
     1) after the view transform, the camera direction vector is always (0, 0, 1)
*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "main.h"
#include "mesh.h"
#include "display.h"
#include "array.h"
#include "matrix.h"
#include "light.h"
#include "upng.h"
#include "camera.h"
#include "player.h"
#include "misc.h"
#include "text.h"
#include "options.h"
#include "log.h"
#include "material.h"
#include "trees.h"
#include "font.h"
#include "game.h"
#include "sound.h"
#include "save_state.h"
#include "debug.h"

#define PLAYER_SPEED_SHIFT_KEY_FACTOR 2.0f
#define PLAYER_ETA_DIRECTION 1.0f /* +1: mouse back looks up, -1: mouse back looks down */
#define MOUSE_SENSITIVITY 0.005f
#define DAY_LENGTH (0.05f * 60.0f * 60.0f / 0.016f) /* hours */

#define FOV_Y_DEG 90.0
#define FOV_Y (FOV_Y_DEG * M_PI / 180.0)
#define ZNEAR 0.05f
#define ZFAR (APPROX_DISTANCE(CHUNK_RENDER_DISTANCE_X, CHUNK_RENDER_DISTANCE_Z) * (CHUNK_SIZE))

uint64_t frame_id = 1; /* start at 1, because all face_draw_stamp values are reset to 0 at object creation time */
char fps_str[16];
uint32_t total_triangles_rendered;
uint32_t total_pixels_rendered;
uint32_t total_chunks_rendered;
float time_of_day; /* keeps time the accumulation of  each delta_time per frame */
char debug_screen_data[1024];

/* game control */
float delta_time;
float move_speed = 2.5f;
float turn_speed = 2.0f;
bool quit = false;
bool is_paused = false;

// debug window
char debug_info_str[4096];

uint8_t color_buffer_reset = 0x20; /* repeat byte for color buffer reset */

/* perspective and camera */
matrix4_t projection_matrix;
matrix4_t view_matrix;

float aspect_ratio_x;
float aspect_ratio_y;
float fov_y;
float fov_x;
float znear;
float zfar;
vector3_t up = {.x = 0, .y = 1, .z = 0}; /* up direction vector for view matrix */

/* sun/sky data */
int32_t skybox_id = 1;
float sun_eta = (M_PI_2); /* sun's eta angle */
float sun_sin_eta = 1.0f;
color_t sky_color = (color_t){.uint32 = 0xFFFF8040};

uint32_t total_chunk_triangles_to_render = 0;

// mining and tools
bool mining = false;
int8_t animation_frame;
int8_t animation_direction = 1;

texture_t biome_map;
texture_t height_map;

void setup(void) {
  
  aspect_ratio_x = (float)window_width  / (float)window_height;
  aspect_ratio_y = (float)window_height / (float)window_width;
  fov_y = FOV_Y;
  fov_x = 2 * atanf(tanf(fov_y / 2.0f) * aspect_ratio_x);
  znear = ZNEAR;
  zfar  = ZFAR;
  projection_matrix = matrix4_gen_perspective(fov_y, aspect_ratio_y, znear, zfar);

  /* reset zbuffer */
  memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t)); /* zbuffer is made of floats and 0x00000000 = 0.0 */

  init_audio();

  initialize_view_frustrum_planes(fov_x, fov_y, znear, zfar);

  /* normalize all light direction vectors to unity */
  initialize_lights();

  /* calculate world chunks */
  allocate_block_memory();
  init_block_materials();
  assign_biomes();
  init_block_map();
  create_world_chunks();
  init_player();

  // save state
  load_all_saved_chunks();

  // block sky light
  reset_light_around_area(0, MAP_NUM_BLOCKS_Y - 1, 0, MAP_NUM_BLOCKS_Z - 1, 0, MAP_NUM_BLOCKS_X - 1);
  seed_initial_sky_light(0, MAP_NUM_BLOCKS_Y - 1, 0, MAP_NUM_BLOCKS_Z - 1, 0, MAP_NUM_BLOCKS_X - 1);
  propagate_sky_light();
  seed_initial_block_light(0, MAP_NUM_BLOCKS_Y - 1, 0, MAP_NUM_BLOCKS_Z - 1, 0, MAP_NUM_BLOCKS_X - 1);
  propagate_block_light();

  play_ambient_sound(SOUND_TYPE_AMBIENT_NATURE_DAY, SOUND_CHANNEL_AMBIENT_NATURE_DAY, -1);
  play_ambient_sound(SOUND_TYPE_WALKING_DIRT, SOUND_CHANNEL_WALKING, -1);
  set_channel_volume(SOUND_CHANNEL_WALKING, 0, false);

  // CREATE OBJECTS
  mesh_t *mesh;
  uint32_t object_id;
  mesh = load_mesh("./assets/obj/blocks/pickaxe.obj", false);
  object_id = create_object(mesh, OBJECT_TYPE_TOOL, true, false, false,
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1.8f, 1.8f, 1.8f}, (vector3_t){0, 0, 0}, (vector3_t){player.position.x, player.position.y - 2, player.position.z},
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
  player_obtain_tool(PLAYER_TOOL_STONE_PICKAXE, object_id);

  mesh = load_mesh("./assets/obj/blocks/axe.obj", false);
  object_id = create_object(mesh, OBJECT_TYPE_TOOL, true, false, false,
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1.8f, 1.8f, 1.8f}, (vector3_t){0, 0, 0}, (vector3_t){player.position.x, player.position.y - 2, player.position.z},
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
  player_obtain_tool(PLAYER_TOOL_STONE_AXE, object_id);

  mesh = load_mesh("./assets/obj/blocks/sword.obj", false);
  object_id = create_object(mesh, OBJECT_TYPE_TOOL, true, false, false,
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1.8f, 1.8f, 1.8f}, (vector3_t){0, 0, 0}, (vector3_t){player.position.x, player.position.y - 2, player.position.z},
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
  player_obtain_tool(PLAYER_TOOL_STONE_SWORD, object_id);

  mesh = load_mesh("./assets/obj/blocks/shovel.obj", false);
  object_id = create_object(mesh, OBJECT_TYPE_TOOL, true, false, false,
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1.8f, 1.8f, 1.8f}, (vector3_t){0, 0, 0}, (vector3_t){player.position.x, player.position.y - 2, player.position.z},
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
  player_obtain_tool(PLAYER_TOOL_STONE_SHOVEL, object_id);


  biome_map.texture_width = MAP_NUM_BLOCKS_X;
  biome_map.texture_height = MAP_NUM_BLOCKS_Z;
  biome_map.texture = malloc(sizeof(color_t) * MAP_NUM_BLOCKS_X * MAP_NUM_BLOCKS_Z);
  if(biome_map.texture == NULL){
    print_log("[ERROR] failed to allocate biome map texture");
    exit(1);
  }
  for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
    for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
      bool is_water = false;
      for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
        if(block_is_water(&block_at(y, z, x))){
          is_water = true;
          break;
        }
      }
      if(is_water)
        biome_map.texture[z * biome_map.texture_width + x].uint32 = DEFAULT_COLOR_BLUE; // blue
      else
        switch(block_at(0, z, x).biome_type){
          case BIOME_TYPE_DESERT:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = 0xFF40DDFF; // sandy
            break;
          case BIOME_TYPE_FOREST:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = 0xFF508020; // blueish green
            break;
          case BIOME_TYPE_GRASSLANDS:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = 0xFF20A040; // light green
            break;
          case BIOME_TYPE_RAIN_FOREST:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = 0xFF206020; // dark green
            break;
          case BIOME_TYPE_SAVANNAH:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = 0xFF20A070; // yellowish green
            break;
          case BIOME_TYPE_SNOW:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = DEFAULT_COLOR_WHITE; // white
            break;
          case BIOME_TYPE_VOLCANIC:
            biome_map.texture[z * biome_map.texture_width + x].uint32 = DEFAULT_COLOR_RED; // red
            break;
        }
    }

  }


  height_map.texture_width = MAP_NUM_BLOCKS_X;
  height_map.texture_height = MAP_NUM_BLOCKS_Z;
  height_map.texture = malloc(sizeof(color_t) * MAP_NUM_BLOCKS_X * MAP_NUM_BLOCKS_Z);
  if(height_map.texture == NULL){
    print_log("[ERROR] failed to allocate biome map texture");
    exit(1);
  }
  for(int32_t x = 0; x < MAP_NUM_BLOCKS_X; x++){
    for(int32_t z = 0; z < MAP_NUM_BLOCKS_Z; z++){
      for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
        if(block_is_water(&block_at(y, z, x))){
          height_map.texture[z * height_map.texture_width + x].uint32 = DEFAULT_COLOR_BLUE; // blue
          break;
        }
        else if(block_is_soil(&block_at(y, z, x)) || block_is_stone(&block_at(y, z, x))){
          float height = (float)(y - WORLD_BASE_SURFACE_LEVEL) / (float)(MAP_NUM_BLOCKS_Y - WORLD_BASE_SURFACE_LEVEL);
          if(height >= 0.0f){
            height_map.texture[z * height_map.texture_width + x].rgb.blue  = height * 255.0f;
            height_map.texture[z * height_map.texture_width + x].rgb.green = height * 255.0f;
            height_map.texture[z * height_map.texture_width + x].rgb.red   = height * 255.0f;
          }
          else{
            height_map.texture[z * height_map.texture_width + x].rgb.blue  = -height * 255.0f;
            height_map.texture[z * height_map.texture_width + x].rgb.green = 0.0f;
            height_map.texture[z * height_map.texture_width + x].rgb.red   = -height * 255.0f;
          }
          break;
        }
      }
    }
  }
}

// poll input and save in input_state struct
void poll_input(input_state_t *input_state){
  SDL_Event event;

  while(SDL_PollEvent(&event)){
    if(event.type == SDL_QUIT){
      input_state->quit_requested = true;
    }

    // detect one shot key downs
    if(event.type == SDL_KEYDOWN && !event.key.repeat){
      input_state->key_pressed[event.key.keysym.scancode] = true;
    }

    // detect mouse wheels
    if(event.type == SDL_MOUSEWHEEL){
      input_state->mouse_wheel_delta += event.wheel.y;
    }

    // detect mouse button down
    if(event.type == SDL_MOUSEBUTTONDOWN){
      if(event.button.button == SDL_BUTTON_LEFT){
        input_state->left_mouse_pressed = true;
      }else if(event.button.button == SDL_BUTTON_RIGHT){
        input_state->right_mouse_pressed = true;
      }
    }

    // detect mouse button up
    if(event.type == SDL_MOUSEBUTTONUP){
      if(event.button.button == SDL_BUTTON_LEFT){
        input_state->left_mouse_released = true;
      }else if(event.button.button == SDL_BUTTON_RIGHT){
        input_state->right_mouse_released = true;
      }
    }

    // detect mouse motion
    if(event.type == SDL_MOUSEMOTION){
      input_state->mouse_x = event.motion.x;
      input_state->mouse_y = event.motion.y;
      input_state->mouse_xrel += event.motion.xrel;
      input_state->mouse_yrel += event.motion.yrel;
    }
  }

  const Uint8 *keyboard_state = SDL_GetKeyboardState(NULL);

  // loop each key to see if pressed
  for(int scancode = 0; scancode < SDL_NUM_SCANCODES; scancode++){
    input_state->key_down[scancode] = keyboard_state[scancode];
  }

  input_state->shift_pressed = keyboard_state[SDL_SCANCODE_LSHIFT] || keyboard_state[SDL_SCANCODE_RSHIFT];
  input_state->ctrl_pressed = keyboard_state[SDL_SCANCODE_LCTRL] || keyboard_state[SDL_SCANCODE_RCTRL];
  input_state->alt_pressed = keyboard_state[SDL_SCANCODE_LALT] || keyboard_state[SDL_SCANCODE_RALT];
}

void handle_game_input_actions(input_state_t *input_state){
  if(input_state->key_pressed[SDL_SCANCODE_ESCAPE]){
    input_state->quit_requested = true;
  }
  if(input_state->key_pressed[SDL_SCANCODE_F12]){
    options.show_debug_info = !options.show_debug_info;
  }
  if(input_state->key_pressed[SDL_SCANCODE_F11]){
    options.see_through = !options.see_through;
  }
  if(input_state->key_pressed[SDL_SCANCODE_F10]){
    options.fly = !options.fly;
  }
}

void handle_player_tool_selection_input(input_state_t *input_state){
  if(input_state->key_down[SDL_SCANCODE_Z]){
    player_set_active_tool_index(0);
  }
  else if(input_state->key_down[SDL_SCANCODE_X]){
    player_set_active_tool_index(1);
  }
  else if(input_state->key_down[SDL_SCANCODE_C]){
    player_set_active_tool_index(2);
  }
  else if(input_state->key_down[SDL_SCANCODE_V]){
    player_set_active_tool_index(3);
  }
  else{
    if(input_state->key_down[SDL_SCANCODE_1]){
      player_set_block_inventory_index(0);
    }
    else if(input_state->key_down[SDL_SCANCODE_2]){
      player_set_block_inventory_index(1);
    }
    else if(input_state->key_down[SDL_SCANCODE_3]){
      player_set_block_inventory_index(2);
    }
    else if(input_state->key_down[SDL_SCANCODE_4]){
      player_set_block_inventory_index(3);
    }
    else if(input_state->key_down[SDL_SCANCODE_5]){
      player_set_block_inventory_index(4);
    }
    else if(input_state->key_down[SDL_SCANCODE_6]){
      player_set_block_inventory_index(5);
    }
    else if(input_state->key_down[SDL_SCANCODE_7]){
      player_set_block_inventory_index(6);
    }
    else if(input_state->key_down[SDL_SCANCODE_8]){
      player_set_block_inventory_index(7);
    }
    else if(input_state->key_down[SDL_SCANCODE_9]){
      player_set_block_inventory_index(8);
    }
    else if(input_state->key_down[SDL_SCANCODE_0]){
      player_set_block_inventory_index(9);
    }
  }
}

void handle_mouse_actions(input_state_t *input_state){
  // MOUSE WHEEL
  if(input_state->mouse_wheel_delta > 0){
    if(player.block_inventory_active_index < player.block_inventory_size - 1)
      player.block_inventory_active_index++;
  }

  if(input_state->mouse_wheel_delta < 0){
    if(player.block_inventory_active_index > 0)
      player.block_inventory_active_index--;
  }

  // RIGHT CLICK: PLACE BLOCK
  if(input_state->right_mouse_pressed){
    if(looked_at_block.hit){
      int32_t player_y_feet = player.position.y;
      int32_t player_y_head = player.position.y + player.height;
      int32_t player_z = player.position.z;
      int32_t player_x = player.position.x;

      int32_t block_y = looked_at_block.block_y + looked_at_block.normal_y;
      int32_t block_z = looked_at_block.block_z + looked_at_block.normal_z;
      int32_t block_x = looked_at_block.block_x + looked_at_block.normal_x;

      if((block_y != player_y_feet && block_y != player_y_head) || block_z != player_z || block_x != player_x){
        if(player.block_inventory[player.block_inventory_active_index].number > 0){

          // play sound
          sound_type_t sound_type = block_materials[player.block_inventory[player.block_inventory_active_index].block_type].placing_sound_type;
          play_transient_sound(SOUND_CHANNEL_MINING, sound_type);

          assign_block_material(block_y, block_z, block_x, player.block_inventory[player.block_inventory_active_index].block_type);
          set_block_orientation(block_y, block_z, block_x, player.position);
          player_use_block();

          // UPDATE SKY LIGHT
          // reset sky light on all blocks around a 32x32x32 area around placed block
          // then reset light in this box of blocks
          // then re-propagate light around all these blocks
          int32_t block_y_start = block_y - MAP_NUM_BLOCKS_Y / 2;
          int32_t block_y_end   = block_y + MAP_NUM_BLOCKS_Y / 2;
          int32_t block_z_start = block_z - 16;
          int32_t block_z_end   = block_z + 16;
          int32_t block_x_start = block_x - 16;
          int32_t block_x_end   = block_x + 16;
          reset_light_around_area(block_y_start, block_y_end, block_z_start, block_z_end, block_x_start, block_x_end);

          // SKY LIGHT
          seed_sky_light_around_area(block_y_start - 1, block_y_end + 1, block_z_start - 1, block_z_end + 1, block_x_start - 1, block_x_end + 1);
          propagate_sky_light();
          
          // BLOCK LIGHT
          seed_block_light_around_area(block_y_start - 1, block_y_end + 1, block_z_start - 1, block_z_end + 1, block_x_start - 1, block_x_end + 1);
          propagate_block_light();


          // SAVE THE CHUNK THAT THE BLOCK IS INSIDE OF
          int32_t chunk_y = block_y / CHUNK_SIZE;
          int32_t chunk_z = block_z / CHUNK_SIZE;
          int32_t chunk_x = block_x / CHUNK_SIZE;
          if(save_chunk(chunk_y, chunk_z, chunk_x)){
            print_log("saved chunk %d:%d:%d", chunk_y, chunk_z, chunk_x);
          }
          else{
            print_log("[ERROR] failed to save chunk %d:%d:%d", chunk_y, chunk_z, chunk_x);
          }
        }
      }
    }
  }

  // LEFT CLICK: START MINING
  if(input_state->left_mouse_pressed){
    mining = true;
  }

  // LEFT RELEASE: STOP MINING
  if(input_state->left_mouse_released){
    mining = false;
    animation_frame = 0;
    animation_direction = 1;

    fadeout_transient_sound_channels();
  }
}

void update_walking_sound(vector3_t movement){
  static bool walking_volume_is_up = false;
  static sound_type_t previous_sound_type = SOUND_TYPE_NONE;

  bool is_walking = (fabsf(movement.x) > 0.01f || fabsf(movement.z) > 0.01f) && !player.on_water && player.on_ground;

  int32_t player_x = (int32_t)player.position.x;
  int32_t player_y = (int32_t)player.position.y - 1;
  int32_t player_z = (int32_t)player.position.z;

  block_t *block = &block_at(player_y, player_z, player_x);
  sound_type_t walking_sound_type = block_materials[block->type].walking_sound_type;

  if(!is_walking || walking_sound_type == SOUND_TYPE_NONE){
    if(walking_volume_is_up){
      fade_channel_volume_to(SOUND_CHANNEL_WALKING, 0, 250);
      walking_volume_is_up = false;
    }
    return;
  }

  bool need_start_sound = previous_sound_type != walking_sound_type || !Mix_Playing(SOUND_CHANNEL_WALKING);

  if(need_start_sound){
    halt_channel(SOUND_CHANNEL_WALKING);
    play_ambient_sound(walking_sound_type, SOUND_CHANNEL_WALKING, -1);
    set_channel_volume(SOUND_CHANNEL_WALKING, 0, false);
    previous_sound_type = walking_sound_type;
    walking_volume_is_up = false;
  }

  // this is needed for when we stop walking and then start again - because when walking stops volume is faded
  if(!walking_volume_is_up){
    fade_channel_volume_to(SOUND_CHANNEL_WALKING, MIX_MAX_VOLUME, 250);
    walking_volume_is_up = true;
  }
}

void update_player_from_input(input_state_t *input_state){

  float move_speed_delta_time = ((input_state->shift_pressed) ? PLAYER_SPEED_SHIFT_KEY_FACTOR * move_speed : move_speed) * delta_time;
  float turn_speed_delta_time = turn_speed * delta_time;

  vector3_t movement = {.x = 0.0f, .y = 0.0f, .z = 0.0f};

  // MOUSE MOTION
  if(input_state->mouse_xrel != 0 || input_state->mouse_yrel != 0){
    player.theta -= MOUSE_SENSITIVITY * input_state->mouse_xrel;
    player.eta   += MOUSE_SENSITIVITY * PLAYER_ETA_DIRECTION * input_state->mouse_yrel;

    if(player.eta > ETA_MAX)
      player.eta = ETA_MAX;

    if(player.eta < -ETA_MAX)
      player.eta = -ETA_MAX;
  }

  if(input_state->key_down[SDL_SCANCODE_LEFT])
    player.theta += turn_speed_delta_time;

  if(input_state->key_down[SDL_SCANCODE_RIGHT])
    player.theta -= turn_speed_delta_time;

  if(input_state->key_down[SDL_SCANCODE_UP]){
    player.eta += turn_speed_delta_time;
    if(player.eta > ETA_MAX) player.eta = ETA_MAX;
  }

  if(input_state->key_down[SDL_SCANCODE_DOWN]){
    player.eta -= turn_speed_delta_time;
    if(player.eta < -ETA_MAX) player.eta = -ETA_MAX;
  }

  player.direction = (vector3_t){
    .x = cosf(player.theta),
    .y = 0.0f,
    .z = sinf(player.theta)
  };

  /* vector orthogonal to player direction: layer.direction (in XZ plane) cross up */
  player_direction_orthogonal_left_right = (vector3_t){
    .x = -player.direction.z * up.y,
    .y =  0.0f,
    .z =  player.direction.x * up.y
  };

  int32_t player_block_y = player.position.y;
  int32_t player_block_z = player.position.z;
  int32_t player_block_x = player.position.x;
  if(block_materials[block_at(player_block_y, player_block_z, player_block_x).type].block_fluid_type == BLOCK_FLUID_WATER ||
    (player_block_y + 1 < MAP_NUM_BLOCKS_Y && block_materials[block_at(player_block_y + 1, player_block_z, player_block_x).type].block_fluid_type == BLOCK_FLUID_WATER)
  ){
    if(player.on_water == false){
      if(player.velocity.y < -4.0f)
        play_transient_sound(SOUND_CHANNEL_FALLING, SOUND_TYPE_FALLING_WATER_STRONG);
      else
        play_transient_sound(SOUND_CHANNEL_FALLING, SOUND_TYPE_FALLING_WATER);
    }
    player.on_water = true;
  }
  else{
    player.on_water = false;
  }

  if(input_state->key_down[SDL_SCANCODE_A]){
    movement.x += player_direction_orthogonal_left_right.x * move_speed_delta_time;
    movement.z += player_direction_orthogonal_left_right.z * move_speed_delta_time;

    if(player.on_water){
      movement.x *= 0.4f;
      movement.z *= 0.4f;
    }
  }

  if(input_state->key_down[SDL_SCANCODE_D]){
    movement.x -= player_direction_orthogonal_left_right.x * move_speed_delta_time;
    movement.z -= player_direction_orthogonal_left_right.z * move_speed_delta_time;

    if(player.on_water){
      movement.x *= 0.4f;
      movement.z *= 0.4f;
    }
  }

  if(input_state->key_down[SDL_SCANCODE_W]){
    movement.x += player.direction.x * move_speed_delta_time;
    movement.z += player.direction.z * move_speed_delta_time;

    if(player.on_water){
      movement.x *= 0.4f;
      movement.z *= 0.4f;
    }
  }

  if(input_state->key_down[SDL_SCANCODE_S]){
    movement.x -= player.direction.x * move_speed_delta_time;
    movement.z -= player.direction.z * move_speed_delta_time;

    if(player.on_water){
      movement.x *= 0.4f;
      movement.z *= 0.4f;
    }
  }

  if(input_state->key_down[SDL_SCANCODE_Q]){
    player.position.y -= 8.0f * move_speed_delta_time;
  }
  if(input_state->key_down[SDL_SCANCODE_E]){
    player.position.y += 8.0f * move_speed_delta_time;
  }

  if(player.on_water){
    if(input_state->key_down[SDL_SCANCODE_SPACE]){
      player.velocity.y = 0.2f * PLAYER_JUMP_VELOCITY;
      player.on_ground = false;
    }
    // gravity
    player.velocity.y -= 0.05f * PLAYER_GRAVITY * delta_time;
    if(player.velocity.y < -0.08f * PLAYER_MAX_FALL_SPEED) player.velocity.y = -0.08f  * PLAYER_MAX_FALL_SPEED;
  }
  else{
    if(input_state->key_down[SDL_SCANCODE_SPACE] && player.on_ground){
      player.velocity.y = PLAYER_JUMP_VELOCITY;
      player.on_ground = false;
    }
    // gravity
    player.velocity.y -= PLAYER_GRAVITY * delta_time;
    if(player.velocity.y < -PLAYER_MAX_FALL_SPEED) player.velocity.y = -PLAYER_MAX_FALL_SPEED;
  }

  movement.y = player.velocity.y * delta_time;
  if(player.on_ground && movement.y <= 0.0f) movement.y = -PLAYER_GROUND_PROBE_SPEED * delta_time;

  if(movement.x != 0.0f) move_player_x(movement.x);
  if(movement.z != 0.0f) move_player_z(movement.z);

  if(!options.fly) if(movement.y != 0.0f) move_player_y(movement.y);

  update_walking_sound(movement);
}

inline void move_player_x(float movement_x){
  if(player.position.x + movement_x < 0) return;
  if(player.position.x + movement_x > MAP_NUM_BLOCKS_X) return;

  vector3_t target_position = player.position;
  target_position.x += movement_x;

  aabb_3d_t target_aabb = make_player_aabb_at_position(target_position);

  int player_vox_x, player_vox_z, player_vox_y;
  player_vox_x = player.position.x;
  player_vox_z = player.position.z;
  player_vox_y = player.position.y;

  int block_x, block_y, block_z;
  block_t *block;

  if(movement_x > 0.0f){
    // y position at player's feet

 //   |   |/___|/
 //   | * |/_x_|/
 //   |   |/___|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min){
        target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

 //   |   |/_x_|/
 //   | * |/_ _|/
 //   |   |/___|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.x_max > block->aabb.x_min){
          target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

 //   |   |/_ _|/
 //   | * |/_ _|/
 //   |   |/_x_|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.x_max > block->aabb.x_min){
          target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +1
 //   |   |/___|/
 //   | * |/_x_|/
 //   |   |/___|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min){
        target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

 //   |   |/_x_|/
 //   | * |/_ _|/
 //   |   |/___|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.x_max > block->aabb.x_min){
          target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

 //   |   |/_ _|/
 //   | * |/_ _|/
 //   |   |/_x_|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.x_max > block->aabb.x_min){
          target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +2
 //   |   |/___|/
 //   | * |/_x_|/
 //   |   |/___|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.x_max > block->aabb.x_min){
          target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

 //   |   |/_x_|/
 //   | * |/_ _|/
 //   |   |/___|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.x_max > block->aabb.x_min){
            target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }

 //   |   |/_ _|/
 //   | * |/_ _|/
 //   |   |/_x_|/ 
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.x_max > block->aabb.x_min){
            target_position.x = block->aabb.x_min - player.radius - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }
  }
  else{
    // y position at player's feet

 //   /___|/   |
 //   /_x_|/ * |
 //   /___|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max){
        target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

 //   /_x_|/   |
 //   /_ _|/ * |
 //   /___|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.x_min < block->aabb.x_max){
          target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

 //   /_ _|/   |
 //   /_ _|/ * |
 //   /_x_|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.x_min < block->aabb.x_max){
          target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +1
 //   /_ _|/   |
 //   /_x_|/ * |
 //   /_ _|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max){
        target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

 //   /_x_|/   |
 //   /_ _|/ * |
 //   /_ _|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.x_min < block->aabb.x_max){
          target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//   /_ _|/   |
//   /_ _|/ * |
//   /_x_|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.x_min < block->aabb.x_max){
          target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +2
 //   /_ _|/   |
 //   /_x_|/ * |
 //   /_ _|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.x_min < block->aabb.x_max){
          target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

 //   /_x_|/   |
 //   /_ _|/ * |
 //   /_ _|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.x_min < block->aabb.x_max){
            target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }

//   /_ _|/   |
//   /_ _|/ * |
//   /_x_|/   | 
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.x_min < block->aabb.x_max){
            target_position.x = block->aabb.x_max + player.radius + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }
  }

  player.position.x = target_position.x;
  update_player_aabb();
}

inline void move_player_z(float movement_z){

  if(player.position.z + movement_z < 0) return;
  if(player.position.z + movement_z > MAP_NUM_BLOCKS_Z) return;

  vector3_t target_position = player.position;
  target_position.z += movement_z;

  aabb_3d_t target_aabb = make_player_aabb_at_position(target_position);

  int player_vox_x, player_vox_z, player_vox_y;

  player_vox_x = player.position.x;
  player_vox_z = player.position.z;
  player_vox_y = player.position.y;

  int block_x, block_y, block_z;
  block_t *block;
  if(movement_z > 0.0f){
    // y position at player's feet

//  /___|/ x |/___|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x;
    block_z = player_vox_z + 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min){
        target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

//  /_ _|/   |/_x_|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_max > block->aabb.z_min){
          target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//  /_x_|/   |/_ _|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_max > block->aabb.z_min){
          target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +1
//  /___|/ x |/___|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min){
        target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

//  /___|/   |/_x_|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_max > block->aabb.z_min){
          target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//  /_x_|/   |/_ _|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_max > block->aabb.z_min){
          target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +2
//  /_ _|/ x |/_ _|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.z_max > block->aabb.z_min){
          target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//  /_ _|/   |/_x_|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.x_max > block->aabb.x_min)
          if(target_aabb.z_max > block->aabb.z_min){
            target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }

//  /_x_|/   |/_ _|/
//  /___|/ * |/___|/
//  /___|/   |/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.x_min < block->aabb.x_max)
          if(target_aabb.z_max > block->aabb.z_min){
            target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }
  }
  else{
    // y position at player's feet

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /___|/ x |/___|/
    block_x = player_vox_x;
    block_z = player_vox_z - 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max){
        target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /___|/   |/_x_|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_min < block->aabb.z_max){
          target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_x_|/   |/_ _|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_min < block->aabb.z_max){
          target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +1
//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_ _|/ x |/_ _|/
    block_x = player_vox_x;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max){
        target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
      }
    }

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_ _|/   |/_x_|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_min < block->aabb.z_max){
          target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_x_|/   |/_ _|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_min < block->aabb.z_max){
          target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

    // y position +2
//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_ _|/ x |/_ _|/
    block_x = player_vox_x;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.z_min < block->aabb.z_max){
          target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
        }
    }

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_ _|/   |/_x_|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.x_max > block->aabb.x_min)
          if(target_aabb.z_min < block->aabb.z_max){
            target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }

//  /_ _|/   |/_ _|/
//  /___|/ * |/___|/
//  /_x_|/   |/_ _|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min)
        if(target_aabb.x_min < block->aabb.x_max)
          if(target_aabb.z_min < block->aabb.z_max){
            target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
    }
  }

  player.position.z = target_position.z;
  update_player_aabb();
}

inline void move_player_y(float movement_y){

  if(player.position.y + movement_y < 0.0f) return;
  if(player.position.y + movement_y > MAP_NUM_BLOCKS_Y) return;

  vector3_t target_position = player.position;
  target_position.y += movement_y;

  aabb_3d_t target_aabb = make_player_aabb_at_position(target_position);

  int player_vox_x, player_vox_z, player_vox_y;

  player_vox_x = player.position.x;
  player_vox_z = player.position.z;
  player_vox_y = player.position.y;

  player.on_ground = false;

  int block_x, block_y, block_z;
  block_t *block;

  if(movement_y < 0.0f){
    // y position at player's feet - 1

//  /___|/___|/___|/
//  /___|/ x |/___|/
//  /___|/___|/___|/
    block_x = player_vox_x;
    block_z = player_vox_z;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_min < block->aabb.y_max){
        if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
        target_position.y = block->aabb.y_max + COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
        player.velocity.y = 0.0f;
        player.on_ground = true;
      }
    }

//  /___|/___|/___|/
//  /_x_|/ * |/___|/
//  /___|/___|/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.y_min < block->aabb.y_max){
          if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
          target_position.y = block->aabb.y_max + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
          player.on_ground = true;
        }
    }

//  /___|/___|/___|/
//  /_ _|/ * |/_x_|/
//  /___|/___|/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.y_min < block->aabb.y_max){
          if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
          target_position.y = block->aabb.y_max + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
          player.on_ground = true;
        }
    }

//  /___|/_x_|/___|/
//  /_ _|/ * |/_ _|/
//  /___|/___|/___|/
    block_x = player_vox_x;
    block_z = player_vox_z + 1;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.y_min < block->aabb.y_max){
          if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
          target_position.y = block->aabb.y_max + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
          player.on_ground = true;
        }
    }

//  /___|/_ _|/___|/
//  /_ _|/ * |/_ _|/
//  /___|/_x_|/___|/
    block_x = player_vox_x;
    block_z = player_vox_z - 1;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.y_min < block->aabb.y_max){
          if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
          target_position.y = block->aabb.y_max + COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
          player.on_ground = true;
        }
    }

//  /_x_|/_ _|/___|/
//  /_ _|/ * |/_ _|/
//  /___|/_ _|/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.y_min < block->aabb.y_max){
            if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
            target_position.y = block->aabb.y_max + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
            player.on_ground = true;
          }
    }

//  /_ _|/_ _|/_x_|/
//  /_ _|/ * |/_ _|/
//  /___|/_ _|/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.y_min < block->aabb.y_max){
            if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
            target_position.y = block->aabb.y_max + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
            player.on_ground = true;
          }
    }

//  /_ _|/_ _|/_ _|/
//  /_ _|/ * |/_ _|/
//  /___|/_ _|/_x_|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.y_min < block->aabb.y_max){
            if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
            target_position.y = block->aabb.y_max + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
            player.on_ground = true;
          }
    }

//  /_ _|/_ _|/_ _|/
//  /_ _|/ * |/_ _|/
//  /_x_|/_ _|/_ _|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y - 1;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.y_min < block->aabb.y_max){
            if(player.velocity.y < -2.0f) play_transient_sound(SOUND_CHANNEL_FALLING, block_materials[block->type].falling_sound_type);
            target_position.y = block->aabb.y_max + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
            player.on_ground = true;
          }
    }


  }
  else{
    // y position at player's feet + 2

//  /___|/___|/___|/
//  /___|/ x |/___|/
//  /___|/___|/___|/
    block_x = player_vox_x;
    block_z = player_vox_z;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.y_max > block->aabb.y_min){
        target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
        target_aabb = make_player_aabb_at_position(target_position);
        player.velocity.y = 0.0f;
      }
    }

//  /___|/___|/___|/
//  /_x_|/ * |/___|/
//  /___|/___|/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.y_max > block->aabb.y_min){
          target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
        }
    }

//  /___|/___|/___|/
//  /_ _|/ * |/_x_|/
//  /___|/___|/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.y_max > block->aabb.y_min){
          target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
        }
    }

//  /___|/_x_|/___|/
//  /_ _|/ * |/_ _|/
//  /___|/___|/___|/
    block_x = player_vox_x;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.y_max > block->aabb.y_min){
          target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
        }
    }

//  /___|/_ _|/___|/
//  /_ _|/ * |/_ _|/
//  /___|/_x_|/___|/
    block_x = player_vox_x;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.y_max > block->aabb.y_min){
          target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
          target_aabb = make_player_aabb_at_position(target_position);
          player.velocity.y = 0.0f;
        }
    }

//  /_x_|/_ _|/___|/
//  /_ _|/ * |/_ _|/
//  /___|/_ _|/___|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.y_max > block->aabb.y_min){
            target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
          }
    }

//  /_ _|/_ _|/_x_|/
//  /_ _|/ * |/_ _|/
//  /___|/_ _|/___|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z + 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.y_max > block->aabb.y_min){
            target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
          }
    }

//  /_ _|/_ _|/_ _|/
//  /_ _|/ * |/_ _|/
//  /___|/_ _|/_x_|/
    block_x = player_vox_x + 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.y_max > block->aabb.y_min){
            target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
          }
    }

//  /_ _|/_ _|/_ _|/
//  /_ _|/ * |/_ _|/
//  /_x_|/_ _|/_ _|/
    block_x = player_vox_x - 1;
    block_z = player_vox_z - 1;
    block_y = player_vox_y + 2;

    block = &block_at(block_y, block_z, block_x);

    if(block_materials[block->type].solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.y_max > block->aabb.y_min){
            target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
            player.velocity.y = 0.0f;
          }
    }
  }

  player.position.y = target_position.y;
  update_player_aabb();

}

void move_player_z_old(float movement_z){
  if(player.position.z + movement_z < 0) return;
  if(player.position.z + movement_z > MAP_NUM_BLOCKS_Z) return;

  if(movement_z == 0.0f) return;

  vector3_t target_position = player.position;
  target_position.z += movement_z;

  aabb_3d_t target_aabb = make_player_aabb_at_position(target_position);

  int start_x, end_x;
  int start_y, end_y;
  int start_z, end_z;

  get_block_range_from_aabb(&target_aabb, &start_x, &end_x, &start_y, &end_y, &start_z, &end_z);

  for(int block_y = start_y; block_y <= end_y; block_y++){
    for(int block_z = start_z; block_z <= end_z; block_z++){
      for(int block_x = start_x; block_x <= end_x; block_x++){
        block_t *block = &block_at(block_y, block_z, block_x);

        if(!block_materials[block->type].solid) continue;
        if(!aabb_3d_overlaps_xy(&target_aabb, &block->aabb)) continue;

        if(movement_z > 0.0f){
          if(target_aabb.z_max > block->aabb.z_min){
            target_position.z = block->aabb.z_min - player.radius - COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
        }
        else{
          if(target_aabb.z_min < block->aabb.z_max){
            target_position.z = block->aabb.z_max + player.radius + COLLISION_EPSILON;
            target_aabb = make_player_aabb_at_position(target_position);
          }
        }
      }
    }
  }

  player.position.z = target_position.z;
  update_player_aabb();
}

void move_player_y_old(float movement_y){
  vector3_t target_position = player.position;
  target_position.y += movement_y;

  if(player.position.y + movement_y < 0) return;
  if(player.position.y + movement_y > MAP_NUM_BLOCKS_Y) return;

  aabb_3d_t target_aabb = make_player_aabb_at_position(target_position);

  int start_x, end_x;
  int start_y, end_y;
  int start_z, end_z;

  get_block_range_from_aabb(&target_aabb, &start_x, &end_x, &start_y, &end_y, &start_z, &end_z);

  player.on_ground = false;

  for(int block_y = start_y; block_y <= end_y; block_y++){
    for(int block_z = start_z; block_z <= end_z; block_z++){
      for(int block_x = start_x; block_x <= end_x; block_x++){
        block_t *block = &block_at(block_y, block_z, block_x);

        if(!block_materials[block->type].solid) continue;
        if(!aabb_3d_overlaps_xz(&target_aabb, &block->aabb)) continue;

        if(movement_y < 0.0f){
          if(player.aabb.y_min >= block->aabb.y_max && target_aabb.y_min <= block->aabb.y_max){
            target_position.y = block->aabb.y_max + COLLISION_EPSILON;
            player.velocity.y = 0.0f;
            player.on_ground = true;
            target_aabb = make_player_aabb_at_position(target_position);
          }
        }
        else if(movement_y > 0.0f){
          if(player.aabb.y_max <= block->aabb.y_min && target_aabb.y_max >= block->aabb.y_min){
            target_position.y = block->aabb.y_min - player.height - COLLISION_EPSILON;
            player.velocity.y = 0.0f;
            target_aabb = make_player_aabb_at_position(target_position);
          }
        }
      }
    }
  }

  player.position.y = target_position.y;
  update_player_aabb();
}

bool resolve_player_against_object_aabb_xz(const collision_aabb_t *box){
  float player_bottom = player.position.y;
  float player_top = player.position.y + player.height;

  if((player_top < box->aabb.y_min || player_bottom > box->aabb.y_max)) return false;

  float nearest_x, nearest_z;
       if(player.position.x < box->aabb.x_min) nearest_x = box->aabb.x_min;
  else if(player.position.x > box->aabb.x_max) nearest_x = box->aabb.x_max;
  else                                         nearest_x = player.position.x;

       if(player.position.z < box->aabb.z_min) nearest_z = box->aabb.z_min;
  else if(player.position.z > box->aabb.z_max) nearest_z = box->aabb.z_max;
  else                                         nearest_z = player.position.z;

  float dx = player.position.x - nearest_x;
  float dz = player.position.z - nearest_z;

  float dist_sq = dx * dx + dz * dz;
  float radius_sq = player.radius * player.radius;

  if(dist_sq >= radius_sq) return false;

  if(dist_sq > 0.000001f){
    float dist = 1.0f / q_rsqrt(dist_sq);
    float push = player.radius - dist;

    player.position.x += (dx / dist) * push;
    player.position.z += (dz / dist) * push;
  }
  else{
    /* Player center is inside the AABB XZ rectangle. Push out nearest side. */
    float pen_left  = fabsf(player.position.x - (float)box->aabb.x_min);
    float pen_right = fabsf((float)box->aabb.x_max - player.position.x);
    float pen_front = fabsf(player.position.z - (float)box->aabb.z_min);
    float pen_back  = fabsf((float)box->aabb.z_max - player.position.z);

    int side = 0;
    float min_pen = pen_left;

    if(pen_right < min_pen){ min_pen = pen_right; side = 1; }
    if(pen_front < min_pen){ min_pen = pen_front; side = 2; }
    if(pen_back  < min_pen){ min_pen = pen_back;  side = 3; }

    switch(side){
      case 0: player.position.x = box->aabb.x_min - player.radius; break;
      case 1: player.position.x = box->aabb.x_max + player.radius; break;
      case 2: player.position.z = box->aabb.z_min - player.radius; break;
      case 3: player.position.z = box->aabb.z_max + player.radius; break;
    }
  }

  return true;
}

/*
RENDER STAGE
  Render sparse/dynamic objects the normal way.
    check for intersection of chunk's 3d dimensional AABB with frustrum:

    classify chunk AABB vs frustum as:
      outside
      intersecting
      inside
      If outside: skip.
      If intersecting:
        render stored faces with normal triangle clipping.
      If inside:
        render stored faces without triangle clipping.
  ** Use a per-frame face stamp to avoid drawing duplicated overlap faces more than once.
  
  ** after all intersecting chunks are found, sort them front to back before rendering.

*/
inline void render_visible_chunks(void) {

  total_triangles_rendered = 0;
  total_pixels_rendered = 0;
  total_chunks_rendered = 0;

  int32_t player_chunk_x = player.position.x / CHUNK_SIZE;
  int32_t player_chunk_y = player.position.y / CHUNK_SIZE;
  int32_t player_chunk_z = player.position.z / CHUNK_SIZE;
  int32_t player_chunk_start_y = player_chunk_y - CHUNK_RENDER_DISTANCE_Y;
  int32_t player_chunk_end_y   = player_chunk_y + CHUNK_RENDER_DISTANCE_Y;
  int32_t player_chunk_start_z = player_chunk_z - CHUNK_RENDER_DISTANCE_Z;
  int32_t player_chunk_end_z   = player_chunk_z + CHUNK_RENDER_DISTANCE_Z;
  int32_t player_chunk_start_x = player_chunk_x - CHUNK_RENDER_DISTANCE_X;
  int32_t player_chunk_end_x   = player_chunk_x + CHUNK_RENDER_DISTANCE_X;
  player_chunk_start_y = clamp_int(player_chunk_start_y, 0, NUM_CHUNK_DIVISIONS_Y - 1);
  player_chunk_end_y   = clamp_int(player_chunk_end_y, 0, NUM_CHUNK_DIVISIONS_Y - 1);
  player_chunk_start_z = clamp_int(player_chunk_start_z, 0, NUM_CHUNK_DIVISIONS_Z - 1);
  player_chunk_end_z   = clamp_int(player_chunk_end_z, 0, NUM_CHUNK_DIVISIONS_Z - 1);
  player_chunk_start_x = clamp_int(player_chunk_start_x, 0, NUM_CHUNK_DIVISIONS_X - 1);
  player_chunk_end_x   = clamp_int(player_chunk_end_x, 0, NUM_CHUNK_DIVISIONS_X - 1);

  chunk_t *visible_chunks[1 + 2*CHUNK_RENDER_DISTANCE_Y + 1 + 2*CHUNK_RENDER_DISTANCE_Z + 1 + 2*CHUNK_RENDER_DISTANCE_X];
  int32_t num_visible_chunks = 0;

  /* collect visible chunks */
  for(int32_t y_plane = player_chunk_start_y; y_plane <= player_chunk_end_y; y_plane++){
    for(int32_t z_row = player_chunk_start_z; z_row <= player_chunk_end_z; z_row++){
      for(int32_t x_col = player_chunk_start_x; x_col <= player_chunk_end_x; x_col++){
        chunk_t *chunk = &chunks[y_plane][z_row][x_col];

        chunk_intersection_mode_t intersection_mode = get_chunk_intersection_mode_fast(chunk);

        if(intersection_mode != CHUNK_INTERSECTION_MODE_INSIDE && intersection_mode != CHUNK_INTERSECTION_MODE_INTERSECTING) continue;
        visible_chunks[num_visible_chunks] = &chunks[y_plane][z_row][x_col];
        num_visible_chunks++;
      }
    }
  }

  // opaque blocks
  for(int32_t chunk_i = 0; chunk_i < num_visible_chunks; chunk_i++){
    render_chunk_blocks(visible_chunks[chunk_i], false);
  }
  // transparent blocks
  for(int32_t chunk_i = 0; chunk_i < num_visible_chunks; chunk_i++){
    render_chunk_blocks(visible_chunks[chunk_i], true);
  }

  if(options.render_chunk_outlines) draw_chunk_outlines();
  //if(options.render_collision_aabb_outlines) draw_collision_aabb_outlines();
}

/*  0 : outside
    1 : inside
   -1 : intersecting

   check each chunk vertex whether inside all planes
     if inside, increment counter
   do this for all vertices
   in the end if count == 8, its fully inside
   if count == 0 its fully outside
   else its intersecting
*/
inline chunk_intersection_mode_t get_chunk_intersection_mode_fast(chunk_t *chunk){
  float x_min = chunk->x_min;
  float x_max = chunk->x_max;
  float y_min = chunk->y_min;
  float y_max = chunk->y_max;
  float z_min = chunk->z_min;
  float z_max = chunk->z_max;

  /* AABB center in world space */
  vector3_t center_world = {
    .x = 0.5f * (x_min + x_max),
    .y = 0.5f * (y_min + y_max),
    .z = 0.5f * (z_min + z_max)
  };

  /* AABB half extents in world space */
  vector3_t extents_world = {
    .x = 0.5f * (x_max - x_min),
    .y = 0.5f * (y_max - y_min),
    .z = 0.5f * (z_max - z_min)
  };

  /* center transformed to view space */
  vector4_t center_view4 = matrix4_mul_vector3(&view_matrix, center_world);
  vector3_t center_view = {
    .x = center_view4.x,
    .y = center_view4.y,
    .z = center_view4.z
  };

  /* transform AABB local axes into view space using only linear 3x3 part */
  vector3_t axis_x_view = {
    .x = view_matrix.element[0][0],
    .y = view_matrix.element[1][0],
    .z = view_matrix.element[2][0]
  };

  vector3_t axis_y_view = {
    .x = view_matrix.element[0][1],
    .y = view_matrix.element[1][1],
    .z = view_matrix.element[2][1]
  };

  vector3_t axis_z_view = {
    .x = view_matrix.element[0][2],
    .y = view_matrix.element[1][2],
    .z = view_matrix.element[2][2]
  };

  bool fully_inside_all_planes = true;

  for(frustrum_plane_t frustrum_plane = FRUSTRUM_LEFT; frustrum_plane < NUM_VIEW_FRUSTRUM_PLANES; frustrum_plane++){
    float s, r;

    if(frustrum_plane == FRUSTRUM_NEAR){
      /* inside if z >= znear */
      s = center_view.z - znear;
      r = fabsf(axis_x_view.z) * extents_world.x +
          fabsf(axis_y_view.z) * extents_world.y +
          fabsf(axis_z_view.z) * extents_world.z;
    }
    else if(frustrum_plane == FRUSTRUM_FAR){
      /* inside if z <= zfar  =>  zfar - z >= 0 */
      s = zfar - center_view.z;
      r = fabsf(axis_x_view.z) * extents_world.x +
          fabsf(axis_y_view.z) * extents_world.y +
          fabsf(axis_z_view.z) * extents_world.z;
    }
    else{
      vector3_t n = view_frustrum_planes[frustrum_plane].normal;

      s = center_view.x * n.x + 
          center_view.y * n.y + 
          center_view.z * n.z;

      r = fabsf(n.x * axis_x_view.x + n.y * axis_x_view.y + n.z * axis_x_view.z) * extents_world.x +
          fabsf(n.x * axis_y_view.x + n.y * axis_y_view.y + n.z * axis_y_view.z) * extents_world.y +
          fabsf(n.x * axis_z_view.x + n.y * axis_z_view.y + n.z * axis_z_view.z) * extents_world.z;
    }

    if(s + r < 0.0f) return CHUNK_INTERSECTION_MODE_OUTSIDE;   /* outside */

    if(s - r < 0.0f) fully_inside_all_planes = false;
  }

  return fully_inside_all_planes ? CHUNK_INTERSECTION_MODE_INSIDE : CHUNK_INTERSECTION_MODE_INTERSECTING;
}

int get_chunk_intersection_mode(chunk_t *chunk){
  double x_min = chunk->x_min;
  double x_max = chunk->x_max;
  double y_min = chunk->y_min;
  double y_max = chunk->y_max;
  double z_min = chunk->z_min;
  double z_max = chunk->z_max;

  vector4_t corners[8];
  corners[0] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_min, y_min, z_min});
  corners[1] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_max, y_min, z_min});
  corners[2] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_min, y_min, z_max});
  corners[3] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_max, y_min, z_max});
  corners[4] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_min, y_max, z_min});
  corners[5] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_max, y_max, z_min});
  corners[6] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_min, y_max, z_max});
  corners[7] = matrix4_mul_vector3(&view_matrix, (vector3_t){x_max, y_max, z_max});

  bool fully_inside_all_planes = true;

  for(frustrum_plane_t frustrum_plane = FRUSTRUM_LEFT; frustrum_plane < NUM_VIEW_FRUSTRUM_PLANES; frustrum_plane++){
    int inside_count = 0;

    for(int i = 0; i < 8; i++)
      if(chunk_vertex_inside_plane(corners[i], frustrum_plane)) inside_count++;

    if(inside_count == 0) return 0;   /* outside */

    if(inside_count != 8) fully_inside_all_planes = false;
  }

  return fully_inside_all_planes ? 1 : -1;
}

bool chunk_vertex_inside_plane(vector4_t vertex, frustrum_plane_t frustrum_plane){
  switch(frustrum_plane){
    case FRUSTRUM_NEAR:
      return vertex.z >= znear;

    case FRUSTRUM_FAR:
      return vertex.z <= zfar;

    case FRUSTRUM_LEFT:
      return (vertex.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x   + vertex.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z)   >= 0.0;

    case FRUSTRUM_RIGHT:
      return (vertex.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x  + vertex.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z)  >= 0.0;

    case FRUSTRUM_BOTTOM:
      return (vertex.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y + vertex.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z) >= 0.0;

    case FRUSTRUM_TOP:
      return (vertex.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y    + vertex.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z)    >= 0.0;
  }

  return false;
}

void render_block_face(
  block_face_t block_face, 
  block_material_t *block_material, 
  bool looked_at, 
  block_orientation_t block_orientation,
  light_intensity_rgb_t light_intensity
){

  /* triangles list for clipped triangles */
  triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES - 2]; /* for a polygon with N vertices, the number of nested triangles is always N-2 */
  vector4_t transformed_vertices[4];

  transformed_vertices[0] = matrix4_mul_vector3(&view_matrix, block_face.points[0]); /* affine transforms so can use this faster multiply */
  transformed_vertices[1] = matrix4_mul_vector3(&view_matrix, block_face.points[1]); 
  transformed_vertices[2] = matrix4_mul_vector3(&view_matrix, block_face.points[2]); 
  transformed_vertices[3] = matrix4_mul_vector3(&view_matrix, block_face.points[3]); 

  /* BACKFACE CULLING */
  /* calculate face normals for backface culling and lighting */
  vector3_t v_a = {transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z};  /*   A               */
  vector3_t v_b = {transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z};  /*  / \    Clockwise */
  vector3_t v_c = {transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z};  /* C---B             */
  /* vectors for cross product */
  vector3_t v_ab = {.x = v_b.x - v_a.x, .y = v_b.y - v_a.y, .z = v_b.z - v_a.z};
  vector3_t v_ac = {.x = v_c.x - v_a.x, .y = v_c.y - v_a.y, .z = v_c.z - v_a.z};
  /* face normal
    cross product between v_ab and v_ac */
  vector3_t v_face_normal = {
    .x = v_ab.y * v_ac.z - v_ab.z * v_ac.y,
    .y = v_ab.z * v_ac.x - v_ab.x * v_ac.z,
    .z = v_ab.x * v_ac.y - v_ab.y * v_ac.x
  };

  /* Backface culling */
  /* camera - v_a ray
    after view_matrix transform, camera position is always at 0, 0, 0 */
  /* calculate dot product between camera ray and orthogonal to find alignment */
  if((block_material->backface_culling && !options.backface_culling_override) || 
      (options.backface_culling_override && options.backface_culling)
  ){
    float dot_face_normal_cam_ray = -(v_face_normal.x * v_a.x + v_face_normal.y * v_a.y + v_face_normal.z * v_a.z);
    if(dot_face_normal_cam_ray < 0.0f) return;
  }

  // FRUSTRUM CLIPPING
  // check whether face is totally outside frustrum, if so, skip face, else apply clipping
  int num_triangles_from_polygon_left = 0;
  int num_triangles_from_polygon_right = 0;
  int total_num_triangles_from_polygon = 0;
  // partially outside or maybe fully inside - clip
  /* this function creates a polygon from the current face, clips against all frustrum planes,
    then re-triangularizes the polygon - it returns the number of generated triangles from the polygon */
  num_triangles_from_polygon_left = frustrum_clip_triangle_and_re_triangularize(
    (vector3_t){transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z},
    (vector3_t){transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z},
    (vector3_t){transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z},
    block_face.uv[0],
    block_face.uv[1],
    block_face.uv[2],
    triangles_after_clipping
  );
  num_triangles_from_polygon_right = frustrum_clip_triangle_and_re_triangularize(
    (vector3_t){transformed_vertices[3].x, transformed_vertices[3].y, transformed_vertices[3].z},
    (vector3_t){transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z},
    (vector3_t){transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z},
    block_face.uv[3],
    block_face.uv[4],
    block_face.uv[5],
    triangles_after_clipping + num_triangles_from_polygon_left
  );

  total_num_triangles_from_polygon = num_triangles_from_polygon_left + num_triangles_from_polygon_right;

  // loop all faces generated from clipped polygon
  for(int t = 0; t < total_num_triangles_from_polygon; t++){
    triangle_t *triangle_after_clipping = &triangles_after_clipping[t];

    /* PROJECTION MATRIX / PERSPECTIVE DIVIDE */
    triangle_t projected_triangle = {0};
    for(int j = 0; j < 3; j++){
      /* optimized multiplication - removed multiplications where matrix elements are 0 */
      vector4_t projected_vertex = matrix4_projection_mul_vector4(&projection_matrix, triangle_after_clipping->points[j]);

      /* w stores old z, so divide by w to create perspective divide */
      if(projected_vertex.w != 0.0f){
        float one_over_projected_vertex_w = 1.0f / projected_vertex.w;
        projected_vertex.x *= one_over_projected_vertex_w;
        projected_vertex.y *= one_over_projected_vertex_w;
        projected_vertex.z *= one_over_projected_vertex_w;
      }

      /* scale vertices by screen width and translate to middle of the screen */
      projected_vertex.x =  projected_vertex.x * (window_width  * 0.5) + (window_width  * 0.5);
      projected_vertex.y = -projected_vertex.y * (window_height * 0.5) + (window_height * 0.5); /* invert y coordinate to account for inverse screen y (screen y grows downward) */

      projected_triangle.points[j] = projected_vertex;
      projected_triangle.uv[j]     = triangle_after_clipping->uv[j];
    }
    
    // TODO: not all materials need different textures for each face. add a condition etc.
    // select texture depending on which face this is
    texture_t *block_face_texture;
    switch(block_face.block_face_type){
      case BLOCK_FACE_TOP:
        block_face_texture = block_material->top_texture;
        break;
      case BLOCK_FACE_BOTTOM:
        block_face_texture = block_material->bottom_texture;
        break;
      case BLOCK_FACE_FRONT:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: block_face_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_BACK:  block_face_texture = block_material->back_texture; break;
          case BLOCK_ORIENTATION_LEFT:  block_face_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: block_face_texture = block_material->right_texture; break;
        }
        break;
      case BLOCK_FACE_BACK:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: block_face_texture = block_material->back_texture; break;
          case BLOCK_ORIENTATION_BACK:  block_face_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_LEFT:  block_face_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: block_face_texture = block_material->right_texture; break;
        }
        break;
      case BLOCK_FACE_LEFT:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: block_face_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_BACK:  block_face_texture = block_material->right_texture; break;
          case BLOCK_ORIENTATION_LEFT:  block_face_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: block_face_texture = block_material->front_texture; break;
        }
        break;
      case BLOCK_FACE_RIGHT:
      default:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: block_face_texture = block_material->right_texture; break;
          case BLOCK_ORIENTATION_BACK:  block_face_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_LEFT:  block_face_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: block_face_texture = block_material->front_texture; break;
        }
        break;
    }

    //projected_triangle.avg_w = (projected_triangle.points[0].w + projected_triangle.points[1].w + projected_triangle.points[2].w) / 3.0f;
    projected_triangle.avg_w = projected_triangle.points[0].w;

    projected_triangle.face_material = (material_t){
      .material_name      = "",
      .alpha_cutoff       = block_material->alpha_cutoff,
      .base_color         = block_material->base_color,
      .blend_mode         = block_material->blend_mode,
      .emissive_color     = block_material->emissive_color,
      .reflects_light     = block_material->reflects_light,
      .two_sided_lighting = false,
      .opacity            = 1.0f,
      .texture            = block_face_texture
    };

    projected_triangle.total_light_intensity = light_intensity;

    /* TRIANGLE RASTERIZATION */
    // if under water then make objects blue shaded
    int32_t player_block_x      = player.position.x;
    int32_t player_block_y_head = player.position.y + player.height * CAMERA_PLAYER_HEIGHT_PERCENTAGE + BLOCK_FACE_SHIFT_DOWN_AMOUNT;
    int32_t player_block_z      = player.position.z;
    block_t *player_block_head  = &block_at(player_block_y_head, player_block_z, player_block_x);
    if(player_block_head->type == BLOCK_TYPE_WATER0){
      projected_triangle.face_material.emissive_color.rgb.green *= 0.5;
      projected_triangle.face_material.emissive_color.rgb.red *= 0.5;
      projected_triangle.total_light_intensity.blue  *= 0.7f;
      projected_triangle.total_light_intensity.green *= 0.3f;
      projected_triangle.total_light_intensity.red   *= 0.3f;
    }

    if(block_material->blend_mode == BLEND_MODE_ALPHA_BLEND){
      projected_triangle.face_material.opacity = 0.65f;
    }

    if(looked_at){
      projected_triangle.total_light_intensity.blue  *= 1.3f + 0.1f;
      projected_triangle.total_light_intensity.green *= 1.3f + 0.1f;
      projected_triangle.total_light_intensity.red   *= 1.3f + 0.1f;
    }

    total_pixels_rendered += rasterize_triangle(&projected_triangle);
    if(options.wireframe_overlay)
      draw_triangle(
        projected_triangle.points[0].x,
        projected_triangle.points[0].y, 
        projected_triangle.points[1].x, 
        projected_triangle.points[1].y, 
        projected_triangle.points[2].x, 
        projected_triangle.points[2].y, 
        (color_t){.uint32 = 0xffffff00}
      );

    total_triangles_rendered++;
    total_chunk_triangles_to_render++;
  } /* for(int t = 0; t < polygon.num_vertices - 2; t++) */
}

void render_chunk_blocks(chunk_t *chunk, bool transparent_only){
  total_chunk_triangles_to_render = 0;

  // find the start and end block indices inside chunk
  int32_t y_plane_start = chunk->y_plane_start;
  int32_t y_plane_end   = chunk->y_plane_start + CHUNK_SIZE;
  int32_t z_row_start   = chunk->z_row_start;
  int32_t z_row_end     = chunk->z_row_start + CHUNK_SIZE;
  int32_t x_col_start   = chunk->x_col_start;
  int32_t x_col_end     = chunk->x_col_start + CHUNK_SIZE;

  // loop through all blocks in chunk
  for(int32_t y_plane = y_plane_start; y_plane < y_plane_end; y_plane++){
    for(int32_t z_row = z_row_start; z_row < z_row_end; z_row++){
      for(int32_t x_col = x_col_start; x_col < x_col_end; x_col++){
        
        block_t *block = &block_at(y_plane, z_row, x_col);
        if(block->type == BLOCK_TYPE_AIR) continue;
        if(
          ( transparent_only && block_materials[block->type].blend_mode != BLEND_MODE_ALPHA_BLEND) || 
          (!transparent_only && block_materials[block->type].blend_mode == BLEND_MODE_ALPHA_BLEND)
        ) continue;
        
        //if(block->type == BLOCK_TYPE_SURFACE_GRASS){
        //
        //}

        bool render_left_block_face   = should_render_face(BLOCK_FACE_LEFT  , y_plane, z_row, x_col);
        bool render_right_block_face  = should_render_face(BLOCK_FACE_RIGHT , y_plane, z_row, x_col);
        bool render_top_block_face    = should_render_face(BLOCK_FACE_TOP   , y_plane, z_row, x_col);
        bool render_bottom_block_face = should_render_face(BLOCK_FACE_BOTTOM, y_plane, z_row, x_col);
        bool render_front_block_face  = should_render_face(BLOCK_FACE_FRONT , y_plane, z_row, x_col);
        bool render_back_block_face   = should_render_face(BLOCK_FACE_BACK  , y_plane, z_row, x_col);

        bool render_left_fluid_connector  = false;
        bool render_right_fluid_connector = false;
        bool render_front_fluid_connector = false;
        bool render_back_fluid_connector  = false;

        if(block_materials[block_at(y_plane, z_row, x_col).type].block_fluid_type != BLOCK_FLUID_NONE){
          render_left_fluid_connector  = should_render_fluid_block_connectors(BLOCK_FACE_LEFT, y_plane, z_row, x_col);
          render_right_fluid_connector = should_render_fluid_block_connectors(BLOCK_FACE_RIGHT, y_plane, z_row, x_col);
          render_front_fluid_connector = should_render_fluid_block_connectors(BLOCK_FACE_FRONT, y_plane, z_row, x_col);
          render_back_fluid_connector  = should_render_fluid_block_connectors(BLOCK_FACE_BACK, y_plane, z_row, x_col);
        }

        block_material_t *block_material = &block_materials[block->type];

        bool block_looked_at_left   = false;
        bool block_looked_at_right  = false;
        bool block_looked_at_top    = false;
        bool block_looked_at_bottom = false;
        bool block_looked_at_front  = false;
        bool block_looked_at_back   = false;
        if(looked_at_block.hit){
          if(y_plane == looked_at_block.block_y && z_row == looked_at_block.block_z && x_col == looked_at_block.block_x){
            if(looked_at_block.normal_x == -1)      block_looked_at_left   = true;
            else if(looked_at_block.normal_x ==  1) block_looked_at_right  = true;
            else if(looked_at_block.normal_y == -1) block_looked_at_bottom = true;
            else if(looked_at_block.normal_y ==  1) block_looked_at_top    = true;
            else if(looked_at_block.normal_z == -1) block_looked_at_front  = true;
            else if(looked_at_block.normal_z ==  1) block_looked_at_back   = true;
          }
        }

        // decide whether the block faces should be shifted down or not (for liquid blocks)
        bool shift_block_faces = should_shift_block_faces(y_plane, z_row, x_col);

        block_orientation_t block_orientation = block_at(y_plane, z_row, x_col).orientation;

        // NOTE: the fluid block connector check must be separate from the face rendering check
        // because a face may not be rendered, but the connector may be needed: i.e top fluid block with a fluid block to its side, the fluid face won't be rendered at all but the connector will

        float face_sky_light, face_block_light, face_light;
        light_intensity_rgb_t light_intensity;

        // LEFT FACE
        if(render_left_block_face || render_left_fluid_connector){
          face_sky_light = block_at(y_plane, z_row, x_col - 1).sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
          face_block_light = block_at(y_plane, z_row, x_col - 1).block_light / MAX_BLOCK_LIGHT_FLOAT;
          face_light = face_sky_light + face_block_light;
          if(face_light < MIN_TOTAL_LIGHT) face_light = MIN_TOTAL_LIGHT;
          if(face_light > 1.0f)   face_light = 1.0f;
          light_intensity = (light_intensity_rgb_t){.blue = face_light, .green = face_light, .red = face_light};
        }
        if(render_left_block_face){
          block_face_t block_face = generate_block_face(BLOCK_FACE_LEFT, y_plane, z_row, x_col);
          if(shift_block_faces){
            block_face.points[1].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[3].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
          }
          render_block_face(block_face, block_material, block_looked_at_left, block_orientation, light_intensity);
        }
        if(render_left_fluid_connector){
          block_face_t fluid_block_connector_face = generate_fluid_top_block_face(BLOCK_FACE_LEFT, y_plane, z_row, x_col);
          render_block_face(fluid_block_connector_face, block_material, block_looked_at_left, block_orientation, light_intensity);
        }

        // RIGHT FACE
        if(render_right_block_face || render_right_fluid_connector){
          face_sky_light = block_at(y_plane, z_row, x_col + 1).sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
          face_block_light = block_at(y_plane, z_row, x_col + 1).block_light / MAX_BLOCK_LIGHT_FLOAT;
          face_light = face_sky_light + face_block_light;
          if(face_light <  MIN_TOTAL_LIGHT) face_light = MIN_TOTAL_LIGHT;
          if(face_light > 1.0f)   face_light = 1.0f;
          light_intensity = (light_intensity_rgb_t){.blue = face_light, .green = face_light, .red = face_light};
        }
        if(render_right_block_face){
          block_face_t block_face = generate_block_face(BLOCK_FACE_RIGHT, y_plane, z_row, x_col);
          if(shift_block_faces){
            block_face.points[1].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[3].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
          }
          render_block_face(block_face, block_material, block_looked_at_right, block_orientation, light_intensity);
        }
        if(render_right_fluid_connector){
          block_face_t fluid_block_connector_face = generate_fluid_top_block_face(BLOCK_FACE_RIGHT, y_plane, z_row, x_col);
          render_block_face(fluid_block_connector_face, block_material, block_looked_at_left, block_orientation, light_intensity);
        }

        // TOP FACE
        if(render_top_block_face && !options.see_through){
          if(block_is_surface_fluid(y_plane, z_row, x_col)){
            light_sample_t light = get_fluid_surface_light(y_plane, z_row, x_col);
            face_sky_light = light.sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
            face_block_light =  light.block_light / MAX_BLOCK_LIGHT_FLOAT;
          }
          else{
            face_sky_light = block_at(y_plane + 1, z_row, x_col).sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
            face_block_light = block_at(y_plane + 1, z_row, x_col).block_light / MAX_BLOCK_LIGHT_FLOAT;
          }
          face_light = face_sky_light + face_block_light;

          if(face_light < MIN_TOTAL_LIGHT) face_light = MIN_TOTAL_LIGHT;
          if(face_light > 1.0f)   face_light = 1.0f;
          light_intensity = (light_intensity_rgb_t){.blue = face_light, .green = face_light, .red = face_light};
          block_face_t block_face = generate_block_face(BLOCK_FACE_TOP, y_plane, z_row, x_col);
          if(shift_block_faces){
            block_face.points[0].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[1].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[2].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[3].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
          }
          render_block_face(block_face, block_material, block_looked_at_top, block_orientation, light_intensity);
        }

        // BOTTOM FACE
        if(render_bottom_block_face && !options.see_through){
          face_sky_light =  block_at(y_plane - 1, z_row, x_col).sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
          face_block_light =  block_at(y_plane - 1, z_row, x_col).block_light / MAX_BLOCK_LIGHT_FLOAT;
          face_light = face_sky_light + face_block_light;
          if(face_light < MIN_TOTAL_LIGHT) face_light = MIN_TOTAL_LIGHT;
          if(face_light > 1.0f)   face_light = 1.0f;
          light_intensity = (light_intensity_rgb_t){.blue = face_light, .green = face_light, .red = face_light};
          block_face_t block_face = generate_block_face(BLOCK_FACE_BOTTOM, y_plane, z_row, x_col);
          render_block_face(block_face, block_material, block_looked_at_bottom, block_orientation, light_intensity);
        }

        // FRONT FACE
        if(render_front_block_face || render_front_fluid_connector){
          face_sky_light =  block_at(y_plane, z_row - 1, x_col).sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
          face_block_light =  block_at(y_plane, z_row - 1, x_col).block_light / MAX_BLOCK_LIGHT_FLOAT;
          face_light = face_sky_light + face_block_light;
          if(face_light < MIN_TOTAL_LIGHT) face_light = MIN_TOTAL_LIGHT;
          if(face_light > 1.0f)   face_light = 1.0f;
          light_intensity = (light_intensity_rgb_t){.blue = face_light, .green = face_light, .red = face_light};
        }
        if(render_front_block_face){
          block_face_t block_face = generate_block_face(BLOCK_FACE_FRONT, y_plane, z_row, x_col);
          if(shift_block_faces){
            block_face.points[1].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[3].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
          }
          render_block_face(block_face, block_material, block_looked_at_front, block_orientation, light_intensity);
        }
        if(render_front_fluid_connector){
          block_face_t fluid_block_connector_face = generate_fluid_top_block_face(BLOCK_FACE_FRONT, y_plane, z_row, x_col);
          render_block_face(fluid_block_connector_face, block_material, block_looked_at_left, block_orientation, light_intensity);
        }

        // BACK FACE
        if(render_back_block_face || render_back_fluid_connector){
          face_sky_light   = block_at(y_plane, z_row + 1, x_col).sky_light / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
          face_block_light = block_at(y_plane, z_row + 1, x_col).block_light / MAX_BLOCK_LIGHT_FLOAT;
          face_light = face_sky_light + face_block_light;
          if(face_light < MIN_TOTAL_LIGHT) face_light = MIN_TOTAL_LIGHT;
          if(face_light > 1.0f)   face_light = 1.0f;
          light_intensity = (light_intensity_rgb_t){.blue = face_light, .green = face_light, .red = face_light};
        }
        if(render_back_block_face){
          block_face_t block_face = generate_block_face(BLOCK_FACE_BACK, y_plane, z_row, x_col);
          if(shift_block_faces){
            block_face.points[1].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
            block_face.points[3].y -= BLOCK_FACE_SHIFT_DOWN_AMOUNT;
          }
          render_block_face(block_face, block_material, block_looked_at_back, block_orientation, light_intensity);
        }
        if(render_back_fluid_connector){
          block_face_t fluid_block_connector_face = generate_fluid_top_block_face(BLOCK_FACE_BACK, y_plane, z_row, x_col);
          render_block_face(fluid_block_connector_face, block_material, block_looked_at_left, block_orientation, light_intensity);
        }
      }
    }
  }
}

void render_object(object_t *object){
  /* triangles list for clipped triangles */
  triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES - 2]; /* for a polygon with N vertices, the number of nested triangles is always N-2 */

  //object->rotation.x += object->angular_velocity.x;
  //object->rotation.y += object->angular_velocity.y;
  //object->rotation.z += object->angular_velocity.z;

  // calculate a vertex for the position of the object (before view matrix so we get actual physical position)
  matrix4_t world_matrix = matrix4_gen_world_matrix(object);

  /* loop through all faces in the face list for object */
  uint32_t num_faces = object->mesh->num_faces;
  for(uint32_t face_idx = 0; face_idx < num_faces; face_idx++){
    face_t face = object->mesh->faces[face_idx];

    /* WORLD MATRIX / VIEW MATRIX TRANSFORM */
    vector3_t face_vertices[3];
    vector4_t transformed_vertices[3];
    
    face_vertices[0] = object->mesh->vertices[face.vertex_indices[0]];
    face_vertices[1] = object->mesh->vertices[face.vertex_indices[1]];
    face_vertices[2] = object->mesh->vertices[face.vertex_indices[2]];

    /* generate the final world matrix, which combines scaling, rotation, and translation, all into one */
    matrix4_t world_view_matrix = matrix4_mul_mat4(&view_matrix, &world_matrix);

    transformed_vertices[0] = matrix4_mul_vector3(&world_view_matrix, face_vertices[0]); /* affine transform so can use this faster multiply */
    transformed_vertices[1] = matrix4_mul_vector3(&world_view_matrix, face_vertices[1]); /* affine transform so can use this faster multiply */
    transformed_vertices[2] = matrix4_mul_vector3(&world_view_matrix, face_vertices[2]); /* affine transform so can use this faster multiply */

    /* BACKFACE CULLING */
    /* calculate face normals for backface culling and lighting */
    vector3_t v_a = {transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z};  /*   A               */
    vector3_t v_b = {transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z};  /*  / \    Clockwise */
    vector3_t v_c = {transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z};  /* C---B             */
    /* vectors for cross product */
    vector3_t v_ab = {.x = v_b.x - v_a.x, .y = v_b.y - v_a.y, .z = v_b.z - v_a.z};
    vector3_t v_ac = {.x = v_c.x - v_a.x, .y = v_c.y - v_a.y, .z = v_c.z - v_a.z};
    /* face normal
      cross product between v_ab and v_ac */
    vector3_t v_face_normal = {
      .x = v_ab.y * v_ac.z - v_ab.z * v_ac.y,
      .y = v_ab.z * v_ac.x - v_ab.x * v_ac.z,
      .z = v_ab.x * v_ac.y - v_ab.y * v_ac.x
    };

    /* Backface culling */
    if((object->backface_culling && !options.backface_culling_override) || 
        (options.backface_culling_override && options.backface_culling)
    ){
      /* camera - v_a ray
        after view_matrix transform, camera position is always at 0, 0, 0 */
      /* calculate dot product between camera ray and orthogonal to find alignment */
      float dot_face_normal_cam_ray = -(v_face_normal.x * v_a.x + v_face_normal.y * v_a.y + v_face_normal.z * v_a.z);
      if(dot_face_normal_cam_ray < 0.0f) continue;
    }

    // FRUSTRUM CLIPPING
    // check whether face is totally outside frustrum, if so, skip face, else apply clipping
    int num_triangles_from_polygon = 0;
    int face_outside_frustrum = check_face_fully_outside_frustrum(transformed_vertices[0], transformed_vertices[1], transformed_vertices[2], znear, zfar);
    if(face_outside_frustrum == 1){
      continue; // totally outside
    }
    else{ // partially outside or maybe fully inside - clip
      /* this function creates a polygon from the current face, clips against all frustrum planes,
        then re-triangularizes the polygon - it returns the number of generated triangles from the polygon */
      num_triangles_from_polygon = frustrum_clip_triangle_and_re_triangularize(
        (vector3_t){transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z},
        (vector3_t){transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z},
        (vector3_t){transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z},
        face.uv[0],
        face.uv[1],
        face.uv[2],
        triangles_after_clipping
      );
    }

    // material for this face - either original mesh material, or object override
    material_t face_material;
    if(object->material_override_flags[face.material_id])
      face_material = object->material_overrides[face.material_id];
    else
      face_material = object->mesh->materials[face.material_id];

    /* LIGHT CALCULATIONS */
    // find total light intensity on this face - this is total intensity per rgb channel
    block_t *block = &block_at((int32_t)player.position.y, (int32_t)player.position.z, (int32_t)player.position.x);
    float sky_light   = (float)block->sky_light   / MAX_SKY_LIGHT_FLOAT * sun_sin_eta;
    float block_light = (float)block->block_light / MAX_BLOCK_LIGHT_FLOAT;
    float object_light = sky_light + block_light;
    if(object_light < MIN_TOTAL_LIGHT) object_light = MIN_TOTAL_LIGHT;
    if(object_light > 1.0f) object_light = 1.0f;
    light_intensity_rgb_t  total_light_intensity = (light_intensity_rgb_t){object_light, object_light, object_light};

    // loop all faces generated from clipped polygon
    for(int t = 0; t < num_triangles_from_polygon; t++){
      triangle_t *triangle_after_clipping = &triangles_after_clipping[t];

      /* PROJECTION MATRIX / PERSPECTIVE DIVIDE */
      triangle_t projected_triangle = {0};
      for(int j = 0; j < 3; j++){
        /* optimized multiplication - removed multiplications where matrix elements are 0 */
        vector4_t projected_vertex = matrix4_projection_mul_vector4(&projection_matrix, triangle_after_clipping->points[j]);

        /* w stores old z, so divide by w to create perspective divide */
        if(projected_vertex.w != 0.0f){
          float one_over_projected_vertex_w = 1.0f / projected_vertex.w;
          projected_vertex.x *= one_over_projected_vertex_w;
          projected_vertex.y *= one_over_projected_vertex_w;
          projected_vertex.z *= one_over_projected_vertex_w;
        }

        /* scale vertices by screen width and translate to middle of the screen */
        projected_vertex.x =  projected_vertex.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_vertex.y = -projected_vertex.y * (window_height * 0.5) + (window_height * 0.5); /* invert y coordinate to account for inverse screen y (screen y grows downward) */

        projected_triangle.points[j] = projected_vertex;
        projected_triangle.uv[j]     = triangle_after_clipping->uv[j];
      }
      
      //projected_triangle.avg_w = (projected_triangle.points[0].w + projected_triangle.points[1].w + projected_triangle.points[2].w) / 3.0f;
      projected_triangle.face_material = face_material;
      projected_triangle.total_light_intensity = total_light_intensity;

      /* TRIANGLE RASTERIZATION */
      //total_pixels_rendered += rasterize_triangle(&projected_triangle);

      // if under water then make objects blue shaded
      int32_t player_block_x      = player.position.x;
      int32_t player_block_y_head = player.position.y + player.height * CAMERA_PLAYER_HEIGHT_PERCENTAGE + BLOCK_FACE_SHIFT_DOWN_AMOUNT;
      int32_t player_block_z      = player.position.z;
      block_t *player_block_head  = &block_at(player_block_y_head, player_block_z, player_block_x);
      if(player_block_head->type == BLOCK_TYPE_WATER0){
        projected_triangle.face_material.emissive_color.rgb.green *= 0.5;
        projected_triangle.face_material.emissive_color.rgb.red *= 0.5;
        projected_triangle.total_light_intensity.blue  *= 0.7f;
        projected_triangle.total_light_intensity.green *= 0.3f;
        projected_triangle.total_light_intensity.red   *= 0.3f;
      }

      total_pixels_rendered += rasterize_triangle(&projected_triangle);
      if(options.wireframe_overlay){
        draw_triangle(
          projected_triangle.points[0].x,
          projected_triangle.points[0].y, 
          projected_triangle.points[1].x, 
          projected_triangle.points[1].y, 
          projected_triangle.points[2].x, 
          projected_triangle.points[2].y, 
          (color_t){.uint32 = 0xffffff00}
        );
      }

      total_triangles_rendered++;
    }
  }
}

void draw_chunk_outlines(void){
  for(uint32_t y_plane = 0; y_plane < NUM_CHUNK_DIVISIONS_Y; y_plane++){
    for(uint32_t z_row = 0; z_row < NUM_CHUNK_DIVISIONS_Z; z_row++){
      for(uint32_t x_col = 0; x_col < NUM_CHUNK_DIVISIONS_X; x_col++){
        /* nnn = xzy */
        vector4_t p000 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_min, .y = chunks[y_plane][z_row][x_col].y_min, .z = chunks[y_plane][z_row][x_col].z_min});
        vector4_t p010 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_min, .y = chunks[y_plane][z_row][x_col].y_min, .z = chunks[y_plane][z_row][x_col].z_max});
        vector4_t p100 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_max, .y = chunks[y_plane][z_row][x_col].y_min, .z = chunks[y_plane][z_row][x_col].z_min});
        vector4_t p110 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_max, .y = chunks[y_plane][z_row][x_col].y_min, .z = chunks[y_plane][z_row][x_col].z_max});
        
        vector4_t p001 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_min, .y = chunks[y_plane][z_row][x_col].y_max, .z = chunks[y_plane][z_row][x_col].z_min});
        vector4_t p011 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_min, .y = chunks[y_plane][z_row][x_col].y_max, .z = chunks[y_plane][z_row][x_col].z_max});
        vector4_t p101 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_max, .y = chunks[y_plane][z_row][x_col].y_max, .z = chunks[y_plane][z_row][x_col].z_min});
        vector4_t p111 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[y_plane][z_row][x_col].x_max, .y = chunks[y_plane][z_row][x_col].y_max, .z = chunks[y_plane][z_row][x_col].z_max});

        vector4_t projected_p000 = matrix4_mul_vector4(&projection_matrix, p000);
        vector4_t projected_p010 = matrix4_mul_vector4(&projection_matrix, p010);
        vector4_t projected_p100 = matrix4_mul_vector4(&projection_matrix, p100);
        vector4_t projected_p110 = matrix4_mul_vector4(&projection_matrix, p110);

        vector4_t projected_p001 = matrix4_mul_vector4(&projection_matrix, p001);
        vector4_t projected_p011 = matrix4_mul_vector4(&projection_matrix, p011);
        vector4_t projected_p101 = matrix4_mul_vector4(&projection_matrix, p101);
        vector4_t projected_p111 = matrix4_mul_vector4(&projection_matrix, p111);

        if(projected_p000.w != 0.0){
          projected_p000.x /= projected_p000.w;
          projected_p000.y /= projected_p000.w;
          projected_p000.z /= projected_p000.w;
        }
        if(projected_p010.w != 0.0){
          projected_p010.x /= projected_p010.w;
          projected_p010.y /= projected_p010.w;
          projected_p010.z /= projected_p010.w;
        }
        if(projected_p100.w != 0.0){
          projected_p100.x /= projected_p100.w;
          projected_p100.y /= projected_p100.w;
          projected_p100.z /= projected_p100.w;
        }
        if(projected_p110.w != 0.0){
          projected_p110.x /= projected_p110.w;
          projected_p110.y /= projected_p110.w;
          projected_p110.z /= projected_p110.w;
        }

        if(projected_p001.w != 0.0){
          projected_p001.x /= projected_p001.w;
          projected_p001.y /= projected_p001.w;
          projected_p001.z /= projected_p001.w;
        }
        if(projected_p011.w != 0.0){
          projected_p011.x /= projected_p011.w;
          projected_p011.y /= projected_p011.w;
          projected_p011.z /= projected_p011.w;
        }
        if(projected_p101.w != 0.0){
          projected_p101.x /= projected_p101.w;
          projected_p101.y /= projected_p101.w;
          projected_p101.z /= projected_p101.w;
        }
        if(projected_p111.w != 0.0){
          projected_p111.x /= projected_p111.w;
          projected_p111.y /= projected_p111.w;
          projected_p111.z /= projected_p111.w;
        }
        
        if(
          projected_p000.x < -1.0f || projected_p010.x < -1.0f || projected_p100.x < -1.0f || projected_p110.x < -1.0f ||
          projected_p000.x > 1.0f || projected_p010.x > 1.0f || projected_p100.x > 1.0f || projected_p110.x > 1.0f ||
          projected_p000.y < -1.0f || projected_p010.y < -1.0f || projected_p100.y < -1.0f || projected_p110.y < -1.0f ||
          projected_p000.y > 1.0f || projected_p010.y > 1.0f || projected_p100.y > 1.0f || projected_p110.y > 1.0f ||
          projected_p000.z < 0.0f || projected_p010.z < 0.0f || projected_p100.z < 0.0f || projected_p110.z < 0.0f ||
          projected_p000.z > 1.0f || projected_p010.z > 1.0f || projected_p100.z > 1.0f || projected_p110.z > 1.0f ||

          projected_p001.x < -1.0f || projected_p011.x < -1.0f || projected_p101.x < -1.0f || projected_p111.x < -1.0f ||
          projected_p001.x > 1.0f  || projected_p011.x > 1.0f  || projected_p101.x > 1.0f  || projected_p111.x > 1.0f  ||
          projected_p001.y < -1.0f || projected_p011.y < -1.0f || projected_p101.y < -1.0f || projected_p111.y < -1.0f ||
          projected_p001.y > 1.0f  || projected_p011.y > 1.0f  || projected_p101.y > 1.0f  || projected_p111.y > 1.0f  ||
          projected_p001.z < 0.0f  || projected_p011.z < 0.0f  || projected_p101.z < 0.0f  || projected_p111.z < 0.0f  ||
          projected_p001.z > 1.0f  || projected_p011.z > 1.0f  || projected_p101.z > 1.0f  || projected_p111.z > 1.0f
        ) continue;

        projected_p000.x =  projected_p000.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p000.y = -projected_p000.y * (window_height * 0.5) + (window_height * 0.5); 
        projected_p010.x =  projected_p010.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p010.y = -projected_p010.y * (window_height * 0.5) + (window_height * 0.5); 
        projected_p100.x =  projected_p100.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p100.y = -projected_p100.y * (window_height * 0.5) + (window_height * 0.5); 
        projected_p110.x =  projected_p110.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p110.y = -projected_p110.y * (window_height * 0.5) + (window_height * 0.5); 

        projected_p001.x =  projected_p001.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p001.y = -projected_p001.y * (window_height * 0.5) + (window_height * 0.5); 
        projected_p011.x =  projected_p011.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p011.y = -projected_p011.y * (window_height * 0.5) + (window_height * 0.5); 
        projected_p101.x =  projected_p101.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p101.y = -projected_p101.y * (window_height * 0.5) + (window_height * 0.5); 
        projected_p111.x =  projected_p111.x * (window_width  * 0.5) + (window_width  * 0.5);
        projected_p111.y = -projected_p111.y * (window_height * 0.5) + (window_height * 0.5); 

        /* bottom */
        draw_line(projected_p000.x, projected_p000.y, projected_p010.x, projected_p010.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p010.x, projected_p010.y, projected_p110.x, projected_p110.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p110.x, projected_p110.y, projected_p100.x, projected_p100.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p100.x, projected_p100.y, projected_p000.x, projected_p000.y, (color_t){.uint32 = 0xFFFFFFFF});

        /* top */
        draw_line(projected_p001.x, projected_p001.y, projected_p011.x, projected_p011.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p011.x, projected_p011.y, projected_p111.x, projected_p111.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p111.x, projected_p111.y, projected_p101.x, projected_p101.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p101.x, projected_p101.y, projected_p001.x, projected_p001.y, (color_t){.uint32 = 0xFFFFFFFF});

        /* sides */
        draw_line(projected_p000.x, projected_p000.y, projected_p001.x, projected_p001.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p100.x, projected_p100.y, projected_p101.x, projected_p101.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p010.x, projected_p010.y, projected_p011.x, projected_p011.y, (color_t){.uint32 = 0xFFFFFFFF});
        draw_line(projected_p110.x, projected_p110.y, projected_p111.x, projected_p111.y, (color_t){.uint32 = 0xFFFFFFFF});
      }
    }
  }
}

void draw_collision_aabb_outlines(void){
  for(uint32_t aabb_id = 0; aabb_id < num_collision_aabbs; aabb_id++){
    /* nnn = xzy */
    vector4_t p000 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_min, .y = collision_aabbs[aabb_id].aabb.y_min, .z = collision_aabbs[aabb_id].aabb.z_min});
    vector4_t p010 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_min, .y = collision_aabbs[aabb_id].aabb.y_min, .z = collision_aabbs[aabb_id].aabb.z_max});
    vector4_t p100 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_max, .y = collision_aabbs[aabb_id].aabb.y_min, .z = collision_aabbs[aabb_id].aabb.z_min});
    vector4_t p110 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_max, .y = collision_aabbs[aabb_id].aabb.y_min, .z = collision_aabbs[aabb_id].aabb.z_max});
    vector4_t p001 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_min, .y = collision_aabbs[aabb_id].aabb.y_max, .z = collision_aabbs[aabb_id].aabb.z_min});
    vector4_t p011 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_min, .y = collision_aabbs[aabb_id].aabb.y_max, .z = collision_aabbs[aabb_id].aabb.z_max});
    vector4_t p101 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_max, .y = collision_aabbs[aabb_id].aabb.y_max, .z = collision_aabbs[aabb_id].aabb.z_min});
    vector4_t p111 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = collision_aabbs[aabb_id].aabb.x_max, .y = collision_aabbs[aabb_id].aabb.y_max, .z = collision_aabbs[aabb_id].aabb.z_max});

    vector4_t projected_p000 = matrix4_mul_vector4(&projection_matrix, p000);
    vector4_t projected_p010 = matrix4_mul_vector4(&projection_matrix, p010);
    vector4_t projected_p100 = matrix4_mul_vector4(&projection_matrix, p100);
    vector4_t projected_p110 = matrix4_mul_vector4(&projection_matrix, p110);
    vector4_t projected_p001 = matrix4_mul_vector4(&projection_matrix, p001);
    vector4_t projected_p011 = matrix4_mul_vector4(&projection_matrix, p011);
    vector4_t projected_p101 = matrix4_mul_vector4(&projection_matrix, p101);
    vector4_t projected_p111 = matrix4_mul_vector4(&projection_matrix, p111);

    if(projected_p000.w != 0.0){
      projected_p000.x /= projected_p000.w;
      projected_p000.y /= projected_p000.w;
      projected_p000.z /= projected_p000.w;
    }
    if(projected_p010.w != 0.0){
      projected_p010.x /= projected_p010.w;
      projected_p010.y /= projected_p010.w;
      projected_p010.z /= projected_p010.w;
    }
    if(projected_p100.w != 0.0){
      projected_p100.x /= projected_p100.w;
      projected_p100.y /= projected_p100.w;
      projected_p100.z /= projected_p100.w;
    }
    if(projected_p110.w != 0.0){
      projected_p110.x /= projected_p110.w;
      projected_p110.y /= projected_p110.w;
      projected_p110.z /= projected_p110.w;
    }

    if(projected_p001.w != 0.0){
      projected_p001.x /= projected_p001.w;
      projected_p001.y /= projected_p001.w;
      projected_p001.z /= projected_p001.w;
    }
    if(projected_p011.w != 0.0){
      projected_p011.x /= projected_p011.w;
      projected_p011.y /= projected_p011.w;
      projected_p011.z /= projected_p011.w;
    }
    if(projected_p101.w != 0.0){
      projected_p101.x /= projected_p101.w;
      projected_p101.y /= projected_p101.w;
      projected_p101.z /= projected_p101.w;
    }
    if(projected_p111.w != 0.0){
      projected_p111.x /= projected_p111.w;
      projected_p111.y /= projected_p111.w;
      projected_p111.z /= projected_p111.w;
    }
    
    if(
      projected_p000.x < -1.0f || projected_p010.x < -1.0f || projected_p100.x < -1.0f || projected_p110.x < -1.0f ||
      projected_p000.x > 1.0f || projected_p010.x > 1.0f || projected_p100.x > 1.0f || projected_p110.x > 1.0f ||
      projected_p000.y < -1.0f || projected_p010.y < -1.0f || projected_p100.y < -1.0f || projected_p110.y < -1.0f ||
      projected_p000.y > 1.0f || projected_p010.y > 1.0f || projected_p100.y > 1.0f || projected_p110.y > 1.0f ||
      projected_p000.z < 0.0f || projected_p010.z < 0.0f || projected_p100.z < 0.0f || projected_p110.z < 0.0f ||
      projected_p000.z > 1.0f || projected_p010.z > 1.0f || projected_p100.z > 1.0f || projected_p110.z > 1.0f ||

      projected_p001.x < -1.0f || projected_p011.x < -1.0f || projected_p101.x < -1.0f || projected_p111.x < -1.0f ||
      projected_p001.x > 1.0f  || projected_p011.x > 1.0f  || projected_p101.x > 1.0f  || projected_p111.x > 1.0f  ||
      projected_p001.y < -1.0f || projected_p011.y < -1.0f || projected_p101.y < -1.0f || projected_p111.y < -1.0f ||
      projected_p001.y > 1.0f  || projected_p011.y > 1.0f  || projected_p101.y > 1.0f  || projected_p111.y > 1.0f  ||
      projected_p001.z < 0.0f  || projected_p011.z < 0.0f  || projected_p101.z < 0.0f  || projected_p111.z < 0.0f  ||
      projected_p001.z > 1.0f  || projected_p011.z > 1.0f  || projected_p101.z > 1.0f  || projected_p111.z > 1.0f
    ) continue;

    projected_p000.x =  projected_p000.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p000.y = -projected_p000.y * (window_height * 0.5) + (window_height * 0.5); 
    projected_p010.x =  projected_p010.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p010.y = -projected_p010.y * (window_height * 0.5) + (window_height * 0.5); 
    projected_p100.x =  projected_p100.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p100.y = -projected_p100.y * (window_height * 0.5) + (window_height * 0.5); 
    projected_p110.x =  projected_p110.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p110.y = -projected_p110.y * (window_height * 0.5) + (window_height * 0.5); 

    projected_p001.x =  projected_p001.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p001.y = -projected_p001.y * (window_height * 0.5) + (window_height * 0.5); 
    projected_p011.x =  projected_p011.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p011.y = -projected_p011.y * (window_height * 0.5) + (window_height * 0.5); 
    projected_p101.x =  projected_p101.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p101.y = -projected_p101.y * (window_height * 0.5) + (window_height * 0.5); 
    projected_p111.x =  projected_p111.x * (window_width  * 0.5) + (window_width  * 0.5);
    projected_p111.y = -projected_p111.y * (window_height * 0.5) + (window_height * 0.5); 

    /* bottom */
    draw_line(projected_p000.x, projected_p000.y, projected_p010.x, projected_p010.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p010.x, projected_p010.y, projected_p110.x, projected_p110.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p110.x, projected_p110.y, projected_p100.x, projected_p100.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p100.x, projected_p100.y, projected_p000.x, projected_p000.y, (color_t){.uint32 = 0xFFFFFFFF});

    /* top */
    draw_line(projected_p001.x, projected_p001.y, projected_p011.x, projected_p011.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p011.x, projected_p011.y, projected_p111.x, projected_p111.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p111.x, projected_p111.y, projected_p101.x, projected_p101.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p101.x, projected_p101.y, projected_p001.x, projected_p001.y, (color_t){.uint32 = 0xFFFFFFFF});

    /* sides */
    draw_line(projected_p000.x, projected_p000.y, projected_p001.x, projected_p001.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p100.x, projected_p100.y, projected_p101.x, projected_p101.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p010.x, projected_p010.y, projected_p011.x, projected_p011.y, (color_t){.uint32 = 0xFFFFFFFF});
    draw_line(projected_p110.x, projected_p110.y, projected_p111.x, projected_p111.y, (color_t){.uint32 = 0xFFFFFFFF});
  }
}

void update_light(void){

  //object_t *skybox = &objects[skybox_id];

  //sun_eta    += M_2_PI * 1.0f / DAY_LENGTH;
  sun_sin_eta = sinf(sun_eta);

  if(sun_sin_eta < 0.0f) sun_sin_eta = 0.0f;

  sky_color.rgb.red   = 0x00 + 0x20 * sun_sin_eta;
  sky_color.rgb.green = 0x00 + 0x80 * sun_sin_eta;
  sky_color.rgb.blue  = 0x08 + 0xE8 * sun_sin_eta;
  
}

void update_camera_from_player(){
  /* camera tracks player */
  camera.position = (vector3_t){
    .x = player.position.x,
    .y = player.position.y + player.height * CAMERA_PLAYER_HEIGHT_PERCENTAGE,
    .z = player.position.z
  };
  camera.direction = (vector3_t){
    .x = cosf(player.eta) * player.direction.x,
    .y = sinf(player.eta),
    .z = cosf(player.eta) * player.direction.z
  };
  camera.theta = player.theta;
  camera.eta = player.eta;

  looked_at_block = raycast_blocks(
    camera.position,
    camera.direction,
    MAX_BLOCK_LOOK_AT_DISTANCE
  );
}

inline uint32_t rasterize_triangle(triangle_t *triangle){
  texture_t *texture = triangle->face_material.texture;

  if(options.projection_mode == TEXT_WIREFRAME){
    draw_triangle(
      triangle->points[0].x, triangle->points[0].y,
      triangle->points[1].x, triangle->points[1].y,
      triangle->points[2].x, triangle->points[2].y,
      (color_t){.uint32 = 0xFFFFFFFF}
    );
    return 0;
  }

  if(options.projection_mode == TEXT_FILLED || texture == NULL){
    return draw_filled_triangle(
      &triangle->points[0],
      &triangle->points[1],
      &triangle->points[2],
      triangle->face_material.base_color,
      triangle->total_light_intensity,
      triangle->face_material.emissive_color,
      triangle->face_material.reflects_light
    );
  }

  float min_x = triangle->points[0].x;
  float max_x = triangle->points[0].x;
  float min_y = triangle->points[0].y;
  float max_y = triangle->points[0].y;

  for(int i = 1; i < 3; i++){
    if(triangle->points[i].x < min_x) min_x = triangle->points[i].x;
    if(triangle->points[i].x > max_x) max_x = triangle->points[i].x;
    if(triangle->points[i].y < min_y) min_y = triangle->points[i].y;
    if(triangle->points[i].y > max_y) max_y = triangle->points[i].y;
  }

  float screen_width = max_x - min_x;
  float screen_height = max_y - min_y;
  float screen_size = screen_width > screen_height ? screen_width : screen_height;

  int mip = 0;

  if(screen_size <= 4.0f)      mip = 2;
  else if(screen_size <= 8.0f) mip = 1;
  else                         mip = 0;

  if(mip >= texture->num_mipmaps) mip = texture->num_mipmaps - 1;

  color_t *mip_mapped_texture = texture->mipmaps[mip];
  int32_t mip_mapped_texture_width = texture->mipmap_widths[mip];
  int32_t mip_mapped_texture_height = texture->mipmap_heights[mip];

  switch(triangle->face_material.blend_mode){
    case BLEND_MODE_OPAQUE:
      return draw_textured_triangle_opaque(
        &triangle->points[0], triangle->uv[0],
        &triangle->points[1], triangle->uv[1],
        &triangle->points[2], triangle->uv[2],
        mip_mapped_texture, mip_mapped_texture_width, mip_mapped_texture_height,
        triangle->total_light_intensity,
        triangle->face_material.emissive_color,
        triangle->face_material.reflects_light
      );

    case BLEND_MODE_ALPHA_TEST:
      return draw_textured_triangle_alpha_test_float(
        &triangle->points[0], triangle->uv[0],
        &triangle->points[1], triangle->uv[1],
        &triangle->points[2], triangle->uv[2],
        mip_mapped_texture, mip_mapped_texture_width, mip_mapped_texture_height,
        triangle->total_light_intensity,
        triangle->face_material.emissive_color,
        triangle->face_material.reflects_light,
        triangle->face_material.alpha_cutoff
      );

    case BLEND_MODE_ALPHA_BLEND:
      return draw_textured_triangle_with_opacity(
        &triangle->points[0], triangle->uv[0],
        &triangle->points[1], triangle->uv[1],
        &triangle->points[2], triangle->uv[2],
        mip_mapped_texture, mip_mapped_texture_width, mip_mapped_texture_height,
        triangle->total_light_intensity,
        triangle->face_material.emissive_color,
        triangle->face_material.reflects_light,
        triangle->face_material.opacity
      );
  }

  return 0;
}

inline void qs_chunk(chunk_sort_key_t *array, int left, int right){
  int i = left;
  int j = right;
  float pivot = array[(left + right) / 2].sort_z;

  while(i <= j){
    while(array[i].sort_z < pivot) i++;
    while(array[j].sort_z > pivot) j--;

    if(i <= j){
      chunk_sort_key_t tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
      i++;
      j--;
    }
  }

  if(left < j) qs_chunk(array, left, j);
  if(i < right) qs_chunk(array, i, right);
}

inline void chunk_quicksort(chunk_sort_key_t *array, int count){
  if(count > 1)
    qs_chunk(array, 0, count - 1);
}

inline void qs_object(object_sort_key_t *array, int left, int right){
  int i = left;
  int j = right;
  float pivot = array[(left + right) / 2].sort_z;

  while(i <= j){
    while(array[i].sort_z < pivot) i++;
    while(array[j].sort_z > pivot) j--;

    if(i <= j){
      object_sort_key_t tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
      i++;
      j--;
    }
  }

  if(left < j) qs_object(array, left, j);
  if(i < right) qs_object(array, i, right);
}

inline void object_quicksort(object_sort_key_t *array, int count){
  if(count > 1)
    qs_object(array, 0, count - 1);
}

inline void qs_triangle(triangle_t *array, int left, int right){
  int i = left;
  int j = right;
  float pivot = array[(left + right) / 2].avg_w;

  while(i <= j){
    while(array[i].avg_w < pivot) i++;
    while(array[j].avg_w > pivot) j--;

    if(i <= j){
      triangle_t tmp = array[i];
      array[i] = array[j];
      array[j] = tmp;
      i++;
      j--;
    }
  }

  if(left < j) qs_triangle(array, left, j);
  if(i < right) qs_triangle(array, i, right);
}

inline void triangle_quicksort(triangle_t *array, int count){
  if(count > 1)
    qs_triangle(array, 0, count - 1);
}

void free_resources(void){
  /* free buffers */
  free(color_buffer);
  free(zbuffer);

  /* free all mesh vertex arrays */
  for(uint32_t i = 0; i < num_meshes; i++){
    array_free(meshes[i].vertices);
    array_free(meshes[i].faces);
  }
}

void process_frame_rate(void) {
  static int64_t previous_frame_time = 0;

  int64_t start_time = SDL_GetTicks64();

  if(previous_frame_time == 0) previous_frame_time = start_time;

  int64_t elapsed = start_time - previous_frame_time;

  if(elapsed < FRAME_TARGET_TIME) SDL_Delay(FRAME_TARGET_TIME - elapsed);

  int64_t end_time = SDL_GetTicks64();
  int64_t frame_time = end_time - previous_frame_time;

  if(frame_time <= 0) frame_time = 1;

  fps = (int)(1000.0 / (double)frame_time);

  delta_time = frame_time / 1000.0f;
  previous_frame_time = end_time;
}

void print_debug_data(){
  if(options.show_debug_info){
    if((frame_id & 0x0F) == 0x0F){
      int32_t player_block_x = player.position.x;
      int32_t player_block_y = player.position.y;
      int32_t player_block_z = player.position.z;
      int32_t player_chunk_x = player_block_x / CHUNK_SIZE;
      int32_t player_chunk_y = player_block_y / CHUNK_SIZE;
      int32_t player_chunk_z = player_block_z / CHUNK_SIZE;
      snprintf(debug_screen_data, 1024, 
        "og: %s, ow: %s, oqs: %s\n"
        "velocity: %.2f:%.2f:%.2f\n"
        "%ut, %upx, %uc, %ufps\n"
        "block coords: %d:%d:%d, chunk coords: %d:%d:%d\n"
        "player pos: %.2f:%.2f:%.2f, look direction: %.2f:%.2f:%.2f, theta/eta: %.2f:%.2f\n"
        "block looked at: %d:%d:%d\n"
        "sl: %d, bl: %d\n"
        "%s",
        player.on_ground    ? "y" : "n",
        player.on_water     ? "y" : "n",
        player.on_quicksand ? "y" : "n",
        player.velocity.x, player.velocity.y, player.velocity.z,
        total_triangles_rendered, total_pixels_rendered, total_chunks_rendered, fps, 
        player_block_x, player_block_y, player_block_z,
        player_chunk_x, player_chunk_y, player_chunk_z,
        player.position.x,  player.position.y,  player.position.z,
        player.direction.x, player.direction.y, player.direction.z,
        player.theta, player.eta,
        looked_at_block.block_x, looked_at_block.block_y, looked_at_block.block_z,
        block_at(player_block_y, player_block_z, player_block_x).sky_light,
        block_at(player_block_y, player_block_z, player_block_x).block_light,
        biomes[block_at((int32_t)player.position.y, (int32_t)player.position.z, (int32_t)player.position.x).biome_type].name
      );
    }
    draw_text(10, 10, debug_screen_data, (color_t){.uint32 = 0xFFFFFFFF}, 1, FONT_6X8);
  }
}

void render_ui(){
  char block_inventory_block_count[8] = "";
  char temperature[8] = "";
  uint32_t block_y_offset = 0;
  float temp;

  // render blocks menu
  //draw_rect(5, window_height - 18, window_width - 10, 18, (color_t){.uint32 = 0xFFFFFFFF});
  int32_t player_block_x = player.position.x;
  int32_t player_block_y = player.position.y;
  int32_t player_block_z = player.position.z;
  temp = block_at(player_block_y, player_block_z, player_block_x).temperature;
  snprintf(temperature, sizeof(temperature), "%.2fC", temp);
  draw_text(22, window_height - 10, temperature, (color_t){.uint32 = 0xFFFFFFFF}, 1, FONT_5X5);

  // go through player's holding blocks list and display it
  for(uint32_t block_i = 0; block_i < player.block_inventory_size; block_i++){
    if(player.block_inventory[block_i].number > 0){
      uint32_t block_y = window_height - 20 - (block_y_offset * 20);
      uint32_t block_x = 0;
      uint32_t text_x = 2;
      uint32_t text_y = block_y + 9;

      draw_texture_flat(block_x, block_y, 20, 20, block_materials[player.block_inventory[block_i].block_type].left_texture);
      // if active block holding is block_i, draw bar vertical bar to left of block
      if(player.block_inventory_active_index == block_i){
        draw_vertical_line(block_x, block_y, 20, (color_t){.uint32 = 0xFFFFFFFF});
      }
      sprintf(block_inventory_block_count, "%u", player.block_inventory[block_i].number);
      draw_text(text_x, text_y, block_inventory_block_count, (color_t){.uint32 = 0xFFFFFFFF}, 1, FONT_5X5);

      block_y_offset++;
    }
  }
}

void process_mining(){
  if(looked_at_block.hit){
    int32_t block_y = looked_at_block.block_y;
    int32_t block_z = looked_at_block.block_z;
    int32_t block_x = looked_at_block.block_x;

    if(mining){
      if(block_materials[block_at(block_y, block_z, block_x).type].mineable){
        player_tool_type_t tool_type = player.tools[player.active_tool_index].tool_type;
        player_tool_definition_t *player_tool_definition = &player_tool_definitions[tool_type];
        int32_t tool_mining_power = player_tool_definition->mining_power;

        int32_t mining_power = tool_mining_power;
        if(player_tool_definition->tool_class != block_materials[block_at(block_y, block_z, block_x).type].preferred_tool_class){
          mining_power = (float)mining_power / 5.0f;
          if(mining_power == 0) mining_power = 1;
        }
        block_at(block_y, block_z, block_x).current_mass -= mining_power;
        //player.tools[player.active_tool_index].durability -= 1;

        if(options.instant_mining) block_at(block_y, block_z, block_x).current_mass = 0;
        if(block_at(block_y, block_z, block_x).current_mass <= 0){

          //block_type_t block_type = block_at(block_y, block_z, block_x).type;
          //play_block_breaking_sound(block_type);

          player_obtain_block(block_at(block_y, block_z, block_x).type);
          assign_block_material(block_y, block_z, block_x, BLOCK_TYPE_AIR);

          // reset sky light on all blocks around an area around placed block
          // then reset light in this box of blocks
          // then re-propagate light around all these blocks
          int32_t block_y_start = block_y - MAP_NUM_BLOCKS_Y / 2;
          int32_t block_y_end   = block_y + MAP_NUM_BLOCKS_Y / 2;
          int32_t block_z_start = block_z - 16;
          int32_t block_z_end   = block_z + 16;
          int32_t block_x_start = block_x - 16;
          int32_t block_x_end   = block_x + 16;
          reset_light_around_area(block_y_start, block_y_end, block_z_start, block_z_end, block_x_start, block_x_end);
          
          // SKY LIGHT
          seed_sky_light_around_area(block_y_start - 1, block_y_end + 1, block_z_start - 1, block_z_end + 1, block_x_start - 1, block_x_end + 1);
          propagate_sky_light();
          
          // BLOCK LIGHT
          seed_block_light_around_area(block_y_start - 1, block_y_end + 1, block_z_start - 1, block_z_end + 1, block_x_start - 1, block_x_end + 1);
          propagate_block_light();

          // SAVE THE CHUNK THAT THE BLOCK IS INSIDE OF
          int32_t chunk_y = block_y / CHUNK_SIZE;
          int32_t chunk_z = block_z / CHUNK_SIZE;
          int32_t chunk_x = block_x / CHUNK_SIZE;

          if(save_chunk(chunk_y, chunk_z, chunk_x)){
            print_log("saved chunk %d:%d:%d", chunk_y, chunk_z, chunk_x);
          }
          else{
            print_log("[ERROR] failed to save chunk %d:%d:%d", chunk_y, chunk_z, chunk_x);
          }
        }
      }
    }
    else{
      block_at(block_y, block_z, block_x).current_mass = block_materials[block_at(block_y, block_z, block_x).type].mass;
    }
  }
}

void render_player_tools(){
  // PLAYER TOOLS
  if(mining && looked_at_block.hit){
    if(animation_direction == 1){
      animation_frame++;
      if(animation_frame >= 1){
        animation_direction = -1;
        sound_type_t sound_type = block_materials[block_at(looked_at_block.block_y, looked_at_block.block_z, looked_at_block.block_x).type].mining_sound_type;
        play_transient_sound(SOUND_CHANNEL_MINING, sound_type);
      }
    }
    else{
      animation_frame--;
      if(animation_frame <= -4){
        animation_direction = 1;

      }
    }
  }

  vector3_t zf = camera.direction;
  /* up cross zf */
  vector3_t xr = {
    .x = up.y * zf.z - up.z * zf.y,
    .y = up.z * zf.x - up.x * zf.z,
    .z = up.x * zf.y - up.y * zf.x
  };
  vector3_normalize(&xr);
  /* zf cross xr */
  vector3_t yu = {
    .x = zf.y * xr.z - zf.z * xr.y,
    .y = zf.z * xr.x - zf.x * xr.z,
    .z = zf.x * xr.y - zf.y * xr.x
  };

  vector3_t tool_direction = vector3_rotate_y(camera.direction, 0.5);

  uint32_t player_active_tool_object_id = player.tools[player.active_tool_index].object_id;
  objects[player_active_tool_object_id].translation.x = camera.position.x + -0.6*yu.x + ZNEAR * camera.direction.x + (mining? 0.28 : 0.1)* tool_direction.x - 0.4f * player_direction_orthogonal_left_right.x ;
  objects[player_active_tool_object_id].translation.y = camera.position.y + -0.6*yu.y + ZNEAR * camera.direction.y + (mining? 0.28 : 0.1)* tool_direction.y;
  objects[player_active_tool_object_id].translation.z = camera.position.z + -0.6*yu.z + ZNEAR * camera.direction.z + (mining? 0.28 : 0.1)* tool_direction.z - 0.4f * player_direction_orthogonal_left_right.z;

  // ROTATION
  objects[player_active_tool_object_id].rotation.y = -camera.theta + M_PI_2 - 0.5;
  float animation_angle = 20*(M_PI) * (float)animation_frame / 7.0f;
  float rotation_x = -0.6 - player.eta + animation_angle * delta_time;
  objects[player_active_tool_object_id].rotation.x = rotation_x;

  // RENDER
  zbuffer_test = false;
  render_object(&objects[player_active_tool_object_id]);
  zbuffer_test = true;
}

bool player_is_inside_cave(void){
  int32_t player_x = player.position.x;
  int32_t player_y = player.position.y;
  int32_t player_z = player.position.z;

  if(player_y > WORLD_WATER_LEVEL){
    return false;
  }

  if(block_at(player_y, player_z, player_x).sky_light > 4){
    return false;
  }

  int32_t air_count = 0;
  int32_t solid_above_count = 0;

  for(int32_t y = player_y - 3; y <= player_y + 3; y++){
    for(int32_t z = player_z - 4; z <= player_z + 4; z++){
      for(int32_t x = player_x - 4; x <= player_x + 4; x++){
        if(y < 0 || y >= MAP_NUM_BLOCKS_Y || z < 0 || z >= MAP_NUM_BLOCKS_Z || x < 0 || x >= MAP_NUM_BLOCKS_X) continue;

        if(block_at(y, z, x).type == BLOCK_TYPE_AIR){
          air_count++;
        }
      }
    }
  }

  // check for blocks above player (y and x). for the x/z tests, we check world boundaries. we check y first because at a world boundary x/z may be outside world, but y is still valid 
  // because y is always below WORLD_WATER_LEVEL
  for(int32_t y = player_y + 2; y <= WORLD_WATER_LEVEL; y++){
    if(block_materials[block_at(y, player_z, player_x).type].solid) solid_above_count++;
    
    if(player_x + 1 >= MAP_NUM_BLOCKS_X || player_z + 1 >= MAP_NUM_BLOCKS_Z) continue;
    if(block_materials[block_at(y, player_z + 1, player_x).type].solid ||
       block_materials[block_at(y, player_z, player_x + 1).type].solid
    ){
      solid_above_count++;
    }

    if(solid_above_count > 6) break;
  }

  if(air_count > 60 && solid_above_count > 6){
    return true;
  }

  return false;
}

void process_water_lava_sounds(){
  // WATER
  int32_t player_y = player.position.y;
  int32_t player_z = player.position.z;
  int32_t player_x = player.position.x;

  for(int32_t y = player_y - 8; y < player_y + 8; y++){
    for(int32_t z = player_z - 8; z < player_z + 8; z++){
      for(int32_t x = player_x - 8; x < player_x + 8; x++){
        if(y >= MAP_NUM_BLOCKS_Y || x >= MAP_NUM_BLOCKS_X || z >= MAP_NUM_BLOCKS_Z || x < 0 || z < 0 || y < 0) continue;

        if(block_is_water(&block_at(y, z, x))){
          play_ambient_sound(SOUND_TYPE_AMBIENT_WATER, SOUND_CHANNEL_AMBIENT_WATER, -1);
          return;
        }
        else if(block_is_lava(&block_at(y, z, x))){
          play_ambient_sound(SOUND_TYPE_AMBIENT_LAVA, SOUND_CHANNEL_AMBIENT_LAVA, -1);
          return;
        }
      }
    }
  }

  fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_WATER, 1024);
  fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_LAVA, 1024);
}

void process_ambient_sounds(){
  static bool night_time = false;

  if(player_is_inside_cave()){
    play_ambient_sound(SOUND_TYPE_AMBIENT_NATURE_CAVE, SOUND_CHANNEL_AMBIENT_NATURE_CAVE, -1);
    fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_NATURE_DAY, 1000);
    fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_NATURE_NIGHT, 1000);
  }
  else{
    fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_NATURE_CAVE, 1000);
    if(!night_time){
      if(sun_sin_eta <= 0.25f){
        night_time = true;
        play_ambient_sound(SOUND_TYPE_AMBIENT_NATURE_NIGHT, SOUND_CHANNEL_AMBIENT_NATURE_NIGHT, -1);
        fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_NATURE_DAY, 1000);
      }
    }
    else{
      if(sun_sin_eta > 0.5f){
        night_time = false;
        play_ambient_sound(SOUND_TYPE_AMBIENT_NATURE_DAY, SOUND_CHANNEL_AMBIENT_NATURE_DAY, -1);
        fadeout_ambient_sound_channel(SOUND_CHANNEL_AMBIENT_NATURE_NIGHT, 1000);
      }
    }
  }

}

void render_maps(){
  int32_t player_z = player.position.z;
  int32_t player_x = player.position.x;

  biome_map.texture[player_z * biome_map.texture_width + player_x].uint32 = 0xFFFFFFFF;
  draw_texture_flat(window_width - 150, 0, 150, 150, &biome_map);

  height_map.texture[player_z * biome_map.texture_width + player_x].uint32 = 0xFF0000FF;
  draw_texture_flat(window_width - 150, 150, 150, 150, &height_map);
}

int main(int argc, char *argv[]){
  (void)argc;
  (void)argv;

  quit = !initialize_window();
  setup();

  for(; !quit; ){
    /* reset zbuffer */
    memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t));
    if(player.position.y < (float)WORLD_BASE_CAVE_LEVEL - 15){
      for(uint32_t pixel_i = 0; pixel_i < total_buffer_pixels; pixel_i++)
        color_buffer[pixel_i].uint32 = 0xFF040404;
    }
    else{
      for(uint32_t pixel_i = 0; pixel_i < total_buffer_pixels; pixel_i++)
        color_buffer[pixel_i] = sky_color;
    }

    process_frame_rate();

    process_ambient_sounds();
    process_water_lava_sounds();
    update_channel_volume_fades();

    input_state_t input_state = {0};
    poll_input(&input_state);

    handle_game_input_actions(&input_state);
    update_player_from_input(&input_state);
    update_camera_from_player();
    handle_player_tool_selection_input(&input_state);   
    handle_mouse_actions(&input_state);

    update_light();

    view_matrix = matrix4_gen_view_matrix(camera.position, camera.direction, up);

    render_visible_chunks();

    render_player_tools();
    process_mining();

    print_debug_data();
    render_ui();
    render_maps();
      
    sdl_render();
    frame_id++; /* one count per frame */

    quit = input_state.quit_requested;
  }

  destroy_window();
  free_resources();

  //debug_print_allocated();

  return 0;
}

void load_skybox(){
  mesh_t *mesh;

  mesh = load_mesh("./assets/obj/blocks/skybox/skybox_cube2.obj", false);
  skybox_id = create_object(mesh, OBJECT_TYPE_SKYBOX, false, false, false, 
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1, 1, 1}, (vector3_t){0.0f, 0.0f, 0.0f}, (vector3_t){0, 0, 0}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0.0001f, 0.0, 0}
  );
}

uint32_t chunk_num_vertices_inside_plane(chunk_t *chunk, frustrum_plane_t frustrum_plane){
  double x_min = chunk->x_min;
  double x_max = chunk->x_max;
  double y_min = chunk->y_min;
  double y_max = chunk->y_max;
  double z_min = chunk->z_min;
  double z_max = chunk->z_max;

  /* nnn = xzy */
  /* now we have all points forming the chunk, transform them into view */
  vector4_t chunk_vert_000_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_min, .y = y_min, .z = z_min});
  vector4_t chunk_vert_100_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_max, .y = y_min, .z = z_min});
  vector4_t chunk_vert_010_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_min, .y = y_min, .z = z_max});
  vector4_t chunk_vert_110_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_max, .y = y_min, .z = z_max});
  vector4_t chunk_vert_001_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_min, .y = y_max, .z = z_min});
  vector4_t chunk_vert_101_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_max, .y = y_max, .z = z_min});
  vector4_t chunk_vert_011_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_min, .y = y_max, .z = z_max});
  vector4_t chunk_vert_111_view = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = x_max, .y = y_max, .z = z_max});

  uint32_t num_inside = 0;

  if(frustrum_plane == FRUSTRUM_NEAR){
    if(chunk_vert_000_view.z > znear) num_inside++;
    if(chunk_vert_100_view.z > znear) num_inside++;
    if(chunk_vert_010_view.z > znear) num_inside++;
    if(chunk_vert_110_view.z > znear) num_inside++;
    if(chunk_vert_001_view.z > znear) num_inside++;
    if(chunk_vert_101_view.z > znear) num_inside++;
    if(chunk_vert_011_view.z > znear) num_inside++;
    if(chunk_vert_111_view.z > znear) num_inside++;

    return num_inside;
  }

  if(frustrum_plane == FRUSTRUM_FAR){
    if(chunk_vert_000_view.z < zfar) num_inside++;
    if(chunk_vert_100_view.z < zfar) num_inside++;
    if(chunk_vert_010_view.z < zfar) num_inside++;
    if(chunk_vert_110_view.z < zfar) num_inside++;
    if(chunk_vert_001_view.z < zfar) num_inside++;
    if(chunk_vert_101_view.z < zfar) num_inside++;
    if(chunk_vert_011_view.z < zfar) num_inside++;
    if(chunk_vert_111_view.z < zfar) num_inside++;

    return num_inside;
  }

  if(frustrum_plane == FRUSTRUM_LEFT){
    if(chunk_vert_000_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_000_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_100_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_100_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_010_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_010_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_110_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_110_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_001_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_001_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_101_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_101_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_011_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_011_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;
    if(chunk_vert_111_view.x * view_frustrum_planes[FRUSTRUM_LEFT].normal.x +
       chunk_vert_111_view.z * view_frustrum_planes[FRUSTRUM_LEFT].normal.z > 0.0) num_inside++;        

    return num_inside;
  }

  if(frustrum_plane == FRUSTRUM_RIGHT){
    if(chunk_vert_000_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_000_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_100_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_100_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_010_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_010_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_110_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_110_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_001_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_001_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_101_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_101_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_011_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_011_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;
    if(chunk_vert_111_view.x * view_frustrum_planes[FRUSTRUM_RIGHT].normal.x +
       chunk_vert_111_view.z * view_frustrum_planes[FRUSTRUM_RIGHT].normal.z > 0.0) num_inside++;

    return num_inside;
  }
  
  if(frustrum_plane == FRUSTRUM_BOTTOM){
    if(chunk_vert_000_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_000_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_100_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_100_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_010_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_010_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_110_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_110_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_001_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_001_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_101_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_101_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_011_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_011_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;
    if(chunk_vert_111_view.y * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.y +
       chunk_vert_111_view.z * view_frustrum_planes[FRUSTRUM_BOTTOM].normal.z > 0.0) num_inside++;

    return num_inside;
  }

  if(frustrum_plane == FRUSTRUM_TOP){
    if(chunk_vert_000_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_000_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_100_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_100_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_010_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_010_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_110_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_110_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_001_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_001_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_101_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_101_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_011_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_011_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;
    if(chunk_vert_111_view.y * view_frustrum_planes[FRUSTRUM_TOP].normal.y +
       chunk_vert_111_view.z * view_frustrum_planes[FRUSTRUM_TOP].normal.z > 0.0) num_inside++;

    return num_inside;
  }

  return 0;
}

void draw_char(
  int x, int y,
  char character, 
  color_t color, int scale, font_type_t type
){
  const uint8_t *glyph = get_font_char(character, type);

  int32_t font_width = fonts[type].width;
  int32_t font_height = fonts[type].height;

  for(int row = 0; row < font_height; row++){
    uint8_t bits = glyph[row];

    for(int col = 0; col < font_width; col++){
      if(bits & (1 << (font_width - 1 - col))){
        for(int scale_y = 0; scale_y < scale; scale_y++){
          int pixel_y = y + row * scale + scale_y;

          if(pixel_y < 0 || pixel_y >= window_height) continue;

          for(int scale_x = 0; scale_x < scale; scale_x++){
            int pixel_x = x + col * scale + scale_x;
            if(pixel_x < 0 || pixel_x >= window_width) continue;
            color_buffer[pixel_y * window_width + pixel_x] = color;
          }
        }
      }
    }
  }
}

void draw_text(
  int x, int y,
  const char *text, 
  color_t color, int scale, font_type_t type
){
  int cursor_x = x;
  int cursor_y = y;

  int32_t font_width = fonts[type].width;
  int32_t font_height = fonts[type].height;

  while(*text != '\0'){
    if(*text == '\n'){
      cursor_x = x;
      cursor_y += (font_height + FONT_VER_SPACING) * scale;
    }
    else{
      draw_char(cursor_x, cursor_y, *text, color, scale, type);
      cursor_x += (font_width + FONT_HOR_SPACING) * scale;
    }

    text++;
  }
}
