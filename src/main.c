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
      CONVERT VOXEL COORDINATES ETC TO -X TO X, ETC. I.E CENTERED AROUND ORIGIN. THIS IS BETTER FOR FLOATING PRECISION
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
#include "debug.h"

#define PLAYER_SPEED_SHIFT_KEY_FACTOR 2.0f
#define PLAYER_ETA_DIRECTION 1.0f /* +1: mouse back looks up, -1: mouse back looks down */
#define MOUSE_SENSITIVITY 0.005f
#define DAY_LENGTH (1.01f * 60.0f * 60.0f / 0.016f) /* hours */

#define FOV_Y_DEG 90.0
#define FOV_Y (FOV_Y_DEG * M_PI / 180.0)
#define ZNEAR 0.05f
#define ZFAR CHUNK_RENDER_DISTANCE * (CHUNK_SIZE * 2) // 300.0f

/* parameters for controlling the absolute ambient light */
#define MIN_AMBIENT_LIGHT_INTENSITY 0.1f
#define AMBIENT_LIGHT_MAX_FACTOR 0.5f
#define MIN_SKYBOX_INTENSITY 0.03f
#define SPECULAR_POWER 16.0f

uint64_t frame_id = 1; /* start at 1, because all face_draw_stamp values are reset to 0 at object creation time */
char fps_str[16];
uint32_t total_triangles_rendered;
uint32_t total_pixels_rendered;
uint32_t total_chunks_rendered;
float time_of_day; /* keeps time the accumulation of  each delta_time per frame */
char debug_screen_data[1024];

/* game control */
float delta_time;
float move_speed = 4.0f;
float turn_speed = 2.0f;
bool quit = false;
bool is_paused = false;

/* player data */
int player_chunk_x;
int player_chunk_z;
int player_chunk_y;
int32_t player_block_x;
int32_t player_block_z;
int32_t player_block_y;

// debug window
char debug_info_str[4096];
int debug_x_collision_start_x, debug_x_collision_start_y, debug_x_collision_start_z;
int debug_x_collision_end_x, debug_x_collision_end_y, debug_x_collision_end_z;
int debug_y_collision_start_y, debug_y_collision_start_y, debug_y_collision_start_z;
int debug_y_collision_end_y, debug_y_collision_end_y, debug_y_collision_end_z;
int debug_z_collision_start_z, debug_z_collision_start_y, debug_z_collision_start_z;
int debug_z_collision_end_z, debug_z_collision_end_y, debug_z_collision_end_z;

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
int32_t skybox_id = -1;
vector3_t sun_direction = {0.0f, 1.0f, 0.0f}; /* world-space sun direction vector */
float sun_eta = (M_PI_2); /* sun's eta angle */

triangle_t triangles_to_render[50000];
uint32_t total_chunk_triangles_to_render = 0;

// mining and tools
bool mining = false;
int32_t pickaxe_id;
int8_t animation_frame;
int8_t animation_direction = 1;

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

  initialize_view_frustrum_planes(fov_x, fov_y, znear, zfar);

  create_light_source(LIGHT_AMBIENT,     (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f}, (vector3_t){.x = 0.0f, .y = 0.0f,  .z = 0.0f}, (color_t){.uint32 = 0xFFFFBBBB}, 0.7f);
  create_light_source(LIGHT_DIRECTIONAL, (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f}, (vector3_t){.x = 1.0f, .y = -1.0f, .z = 1.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 1.0f);

  /* normalize all light direction vectors to unity */
  initialize_lights();

  /* calculate world chunks */
  init_block_materials();
  init_block_map();
  calculate_world_bounds();
  create_world_chunks();
  init_player();

  // CREATE OBJECTS
  mesh_t *mesh;
  mesh = load_mesh("./assets/obj/blocks/pickaxe3.obj", false);
  pickaxe_id = create_object(mesh, OBJECT_TYPE_TOOL, true, false, false,
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1.8f, 1.8f, 1.8f}, (vector3_t){0, 0, 0}, (vector3_t){player.position.x, player.position.y - 2, player.position.z},
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
}

void process_input(void){
  SDL_Event event;

  while(SDL_PollEvent(&event)){
    if(event.type == SDL_QUIT){
      quit = true;
      continue;
    }

    if(event.type == SDL_KEYDOWN && !event.key.repeat){
      SDL_Keycode keycode = event.key.keysym.sym;
      Uint16 modifiers = event.key.keysym.mod;

      if(keycode == SDLK_ESCAPE){
        if(console.is_open){
          console.is_open = false;
          SDL_StopTextInput();
        }
        else{
          quit = true;
        }
        continue;
      }

      if(keycode == SDLK_BACKQUOTE && (modifiers & KMOD_CTRL)){
        console.is_open = !console.is_open;

        if(console.is_open){
          SDL_StartTextInput();
        }
        else{
          SDL_StopTextInput();
        }

        continue;
      }

      if(keycode == SDLK_F12){
        options.show_debug_info = !options.show_debug_info;
        continue;
      }

    }

    if(console.is_open){
      process_console_input(&event);
    }
    else{
      process_game_input(&event);
    }
  }
}

void process_game_input(SDL_Event *event){

  // KEYBOARD
  if(event->type == SDL_KEYDOWN && !event->key.repeat){
    SDL_Keycode keycode = event->key.keysym.sym;
    //Uint16 modifiers = event->key.keysym.mod;

    if(keycode == SDLK_1){
      player_set_block_inventory_index(0);
    }
    if(keycode == SDLK_2){
      player_set_block_inventory_index(1);
    }
    if(keycode == SDLK_3){
      player_set_block_inventory_index(2);
    }
    if(keycode == SDLK_4){
      player_set_block_inventory_index(3);
    }
    if(keycode == SDLK_5){
      player_set_block_inventory_index(4);
    }
    if(keycode == SDLK_6){
      player_set_block_inventory_index(5);
    }
    if(keycode == SDLK_7){
      player_set_block_inventory_index(6);
    }
    if(keycode == SDLK_8){
      player_set_block_inventory_index(7);
    }
    if(keycode == SDLK_9){
      player_set_block_inventory_index(8);
    }
    if(keycode == SDLK_0){
      player_set_block_inventory_index(9);
    }
  }

  // MOUSE WHEEL
  if(event->type == SDL_MOUSEWHEEL){
    if(event->wheel.y > 0){
      if(player.block_inventory_active_index < player.block_inventory_size - 1)
        player.block_inventory_active_index++;
    }
    if(event->wheel.y < 0){
      if(player.block_inventory_active_index > 0)
        player.block_inventory_active_index--;
    }
  }

  // MOUSE BUTTONS
  if(event->type == SDL_MOUSEMOTION){
    player.theta  -= MOUSE_SENSITIVITY                        * event->motion.xrel;
    player.eta    += MOUSE_SENSITIVITY * PLAYER_ETA_DIRECTION * event->motion.yrel;
    if(player.eta  >  ETA_MAX) player.eta =  ETA_MAX;
    if(player.eta  < -ETA_MAX) player.eta = -ETA_MAX;
  }
  if(event->type == SDL_MOUSEBUTTONDOWN){
    if(event->button.button == SDL_BUTTON_RIGHT){
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
            assign_block_material(block_y, block_z, block_x, player.block_inventory[player.block_inventory_active_index].block_type);
            set_block_orientation(block_y, block_z, block_x, player.position);
            block_at(block_y, block_z, block_x).solid = true;
            block_at(block_y, block_z, block_x).mining_amount = block_at(block_y, block_z, block_x).block_material->mining_amount;
            player_use_block();
          }
        }
      }
    }
    else if(event->button.button == SDL_BUTTON_LEFT){
      mining = true;
    }
  }
  else if(event->type == SDL_MOUSEBUTTONUP){
    if(event->button.button == SDL_BUTTON_LEFT){
      mining = false;
      animation_frame = 0;
      animation_direction = 1;
    }
  }
}

void process_console_input(SDL_Event *event){
  if(event->type == SDL_KEYDOWN && !event->key.repeat){
    switch(event->key.keysym.sym){

      case SDLK_BACKSPACE:
        if(console.cursor_position > 0){
          memmove(&console.input_buffer[console.cursor_position - 1], &console.input_buffer[console.cursor_position], console.input_length - console.cursor_position + 1);
          console.cursor_position--;
          console.input_length--;
        }
        break;

      case SDLK_DELETE:
        if(console.cursor_position < console.input_length){
          memmove(&console.input_buffer[console.cursor_position], &console.input_buffer[console.cursor_position + 1], console.input_length - console.cursor_position);
          console.input_length--;
        }
        break;

      case SDLK_LEFT:
        if(console.cursor_position > 0){
          console.cursor_position--;
        }
        break;

      case SDLK_RIGHT:
        if(console.cursor_position < console.input_length){
          console.cursor_position++;
        }
        break;

      case SDLK_HOME:
        console.cursor_position = 0;
        break;

      case SDLK_END:
        console.cursor_position = console.input_length;
        break;

      case SDLK_RETURN:
        run_console_command(console.input_buffer);

        console.input_length = 0;
        console.cursor_position = 0;
        console.input_buffer[0] = '\0';
        break;
    }
  }

  if(event->type == SDL_TEXTINPUT){
    const char *input_text = event->text.text;
    int text_length = strlen(input_text);

    if(console.input_length + text_length < (int)sizeof(console.input_buffer) - 1){
      memmove(&console.input_buffer[console.cursor_position + text_length], &console.input_buffer[console.cursor_position], console.input_length - console.cursor_position + 1);
      memcpy(&console.input_buffer[console.cursor_position], input_text, text_length);
      console.cursor_position += text_length;
      console.input_length += text_length;
    }
  }
}

void render_console(){
  draw_filled_rect(0, 0, window_width, 80, (color_t){.uint32 = 0xFF000000});
  draw_text(10, 10, "> ", (color_t){.uint32 = 0xFFFFFFFF}, 1, FONT_5X5);
  draw_text(20, 10, console.input_buffer, (color_t){.uint32 = 0xFFFFFFFF}, 1, FONT_5X5);
}

void run_console_command(){
  if(!strcmp(console.input_buffer, "mp")){
    options.projection_mode = TEXT_PERSPECTIVE_CORRECT; 
  }
  else if(!strcmp(console.input_buffer, "ma")){
    options.projection_mode = TEXT_AFFINE; 
  }
  else if(!strcmp(console.input_buffer, "mf")){
    options.projection_mode = TEXT_FILLED; 
  }
  else if(!strcmp(console.input_buffer, "mw")){
    options.projection_mode = TEXT_WIREFRAME; 
  }
  else if(!strcmp(console.input_buffer, "ca")){
    options.render_collision_aabb_outlines = !options.render_collision_aabb_outlines; 
  }
  else if(!strcmp(console.input_buffer, "co")){
    options.render_chunk_outlines = !options.render_chunk_outlines; 
  }
  else if(!strcmp(console.input_buffer, "bfc ovrd")){
   options.backface_culling_override = !options.backface_culling_override; 
  }
  else if(!strcmp(console.input_buffer, "bfc")){
   options.backface_culling = !options.backface_culling; 
  }
  else if(!strcmp(console.input_buffer, "wireframe")){
    options.wireframe_overlay = !options.wireframe_overlay; 
  }
  else if(!strcmp(console.input_buffer, "dither")){
    options.dither = !options.dither;
  }
  else if(!strcmp(console.input_buffer, "cst")){
    options.chunk_sort = !options.chunk_sort;
  }
  else if(!strcmp(console.input_buffer, "im")){
    options.instant_mining = !options.instant_mining;
  }
}

void move_player(void){
  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  bool shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

  float move_speed_delta_time = ((shift) ? PLAYER_SPEED_SHIFT_KEY_FACTOR * move_speed : move_speed) * delta_time;
  float turn_speed_delta_time = turn_speed * delta_time;

  vector3_t movement = {
    .x = 0.0f,
    .y = 0.0f,
    .z = 0.0f
  };

  if(keys[SDL_SCANCODE_LEFT])
    player.theta += turn_speed_delta_time;

  if(keys[SDL_SCANCODE_RIGHT])
    player.theta -= turn_speed_delta_time;

  if(keys[SDL_SCANCODE_UP]){
    player.eta += turn_speed_delta_time;
    if(player.eta > ETA_MAX) player.eta = ETA_MAX;
  }

  if(keys[SDL_SCANCODE_DOWN]){
    player.eta -= turn_speed_delta_time;
    if(player.eta < -ETA_MAX) player.eta = -ETA_MAX;
  }

  if(keys[SDL_SCANCODE_A]){
    movement.x += player_direction_orthogonal_left_right.x * move_speed_delta_time;
    movement.z += player_direction_orthogonal_left_right.z * move_speed_delta_time;
  }

  if(keys[SDL_SCANCODE_D]){
    movement.x -= player_direction_orthogonal_left_right.x * move_speed_delta_time;
    movement.z -= player_direction_orthogonal_left_right.z * move_speed_delta_time;
  }

  if(keys[SDL_SCANCODE_W]){
    movement.x += player.direction.x * move_speed_delta_time;
    movement.z += player.direction.z * move_speed_delta_time;
  }

  if(keys[SDL_SCANCODE_S]){
    movement.x -= player.direction.x * move_speed_delta_time;
    movement.z -= player.direction.z * move_speed_delta_time;
  }

  if(keys[SDL_SCANCODE_SPACE] && player.on_ground){
    player.velocity.y = PLAYER_JUMP_VELOCITY;
    player.on_ground = false;
  }

  // gravity
  player.velocity.y -= PLAYER_GRAVITY * delta_time;
  if(player.velocity.y < -PLAYER_MAX_FALL_SPEED) player.velocity.y = -PLAYER_MAX_FALL_SPEED;
  movement.y = player.velocity.y * delta_time;
  if(player.on_ground && movement.y <= 0.0f) movement.y = -PLAYER_GROUND_PROBE_SPEED * delta_time;

  move_player_x(movement.x);
  move_player_z(movement.z);
  move_player_y(movement.y);
}

inline void move_player_x(float movement_x){
  if(movement_x == 0.0f) return;

  if(player.position.x + movement_x < 0) return;
  if(player.position.x + movement_x > MAP_NUM_VOXELS_X) return;

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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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
  if(movement_z == 0.0f) return;

  if(player.position.z + movement_z < 0) return;
  if(player.position.z + movement_z > MAP_NUM_VOXELS_Z) return;

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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

  if(movement_y == 0.0f) return;

  if(player.position.y + movement_y < 0.0f) return;
  if(player.position.y + movement_y > MAP_NUM_VOXELS_Y) return;

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

    if(block->solid){
      if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.z_max > block->aabb.z_min)
        if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.z_min < block->aabb.z_max)
        if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_max > block->aabb.z_min)
          if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.x_max > block->aabb.x_min)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
      if(target_aabb.x_min < block->aabb.x_max)
        if(target_aabb.z_min < block->aabb.z_max)
          if(target_aabb.y_min < block->aabb.y_max){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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

    if(block->solid){
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
  if(player.position.z + movement_z > MAP_NUM_VOXELS_Z) return;

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

        if(block->solid == false) continue;
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
  if(player.position.y + movement_y > MAP_NUM_VOXELS_Y) return;

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

        if(block->solid == false) continue;
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
  /* reset color buffer */
  for(uint32_t pixel_i = 0; pixel_i < total_buffer_pixels; pixel_i++){
    color_buffer[pixel_i] = (color_t){.uint32 = 0xFFFF8050};
  }
  //memset(color_buffer, color_buffer_reset, total_buffer_pixels * sizeof(uint32_t));

  /* reset zbuffer */
  memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t));
  
  view_matrix = matrix4_gen_view_matrix(camera.position, camera.direction, up);

  total_triangles_rendered = 0;
  total_pixels_rendered = 0;
  total_chunks_rendered = 0;

  int32_t player_chunk_start_y = player_chunk_y - CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_end_y   = player_chunk_y + CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_start_z = player_chunk_z - CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_end_z   = player_chunk_z + CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_start_x = player_chunk_x - CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_end_x   = player_chunk_x + CHUNK_RENDER_DISTANCE;
  player_chunk_start_y = clamp_int(player_chunk_start_y, 0, NUM_CHUNK_DIVISIONS_Y - 1);
  player_chunk_end_y   = clamp_int(player_chunk_end_y, 0, NUM_CHUNK_DIVISIONS_Y - 1);
  player_chunk_start_z = clamp_int(player_chunk_start_z, 0, NUM_CHUNK_DIVISIONS_Z - 1);
  player_chunk_end_z   = clamp_int(player_chunk_end_z, 0, NUM_CHUNK_DIVISIONS_Z - 1);
  player_chunk_start_x = clamp_int(player_chunk_start_x, 0, NUM_CHUNK_DIVISIONS_X - 1);
  player_chunk_end_x   = clamp_int(player_chunk_end_x, 0, NUM_CHUNK_DIVISIONS_X - 1);

  /* collect visible chunks */
  for(int32_t y_plane = player_chunk_start_y; y_plane <= player_chunk_end_y; y_plane++){
    for(int32_t z_row = player_chunk_start_z; z_row <= player_chunk_end_z; z_row++){
      for(int32_t x_col = player_chunk_start_x; x_col <= player_chunk_end_x; x_col++){
        chunk_t *chunk = &chunks[y_plane][z_row][x_col];

        chunk_intersection_mode_t intersection_mode = get_chunk_intersection_mode_fast(chunk);

        if(intersection_mode != CHUNK_INTERSECTION_MODE_INSIDE && intersection_mode != CHUNK_INTERSECTION_MODE_INTERSECTING) continue;
        render_chunk_blocks(y_plane, z_row, x_col);

        total_chunks_rendered++;
      }
    }
  }

  if(options.render_chunk_outlines) draw_chunk_outlines();
  if(options.render_collision_aabb_outlines) draw_collision_aabb_outlines();
}

// sort chunks front to back and render
inline void render_visible_chunks_sorted(void) {
  /* reset color buffer */
  for(uint32_t pixel_i = 0; pixel_i < total_buffer_pixels; pixel_i++){
    color_buffer[pixel_i] = (color_t){.uint32 = 0xFFFF8050};
  }
  //memset(color_buffer, color_buffer_reset, total_buffer_pixels * sizeof(uint32_t));

  /* reset zbuffer */
  memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t));

  view_matrix = matrix4_gen_view_matrix(camera.position, camera.direction, up);

  total_triangles_rendered = 0;
  total_pixels_rendered = 0;
  total_chunks_rendered = 0;

  int32_t player_chunk_start_y = player_chunk_y - CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_end_y   = player_chunk_y + CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_start_z = player_chunk_z - CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_end_z   = player_chunk_z + CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_start_x = player_chunk_x - CHUNK_RENDER_DISTANCE;
  int32_t player_chunk_end_x   = player_chunk_x + CHUNK_RENDER_DISTANCE;
  player_chunk_start_y = clamp_int(player_chunk_start_y, 0, NUM_CHUNK_DIVISIONS_Y - 1);
  player_chunk_end_y   = clamp_int(player_chunk_end_y, 0, NUM_CHUNK_DIVISIONS_Y - 1);
  player_chunk_start_z = clamp_int(player_chunk_start_z, 0, NUM_CHUNK_DIVISIONS_Z - 1);
  player_chunk_end_z   = clamp_int(player_chunk_end_z, 0, NUM_CHUNK_DIVISIONS_Z - 1);
  player_chunk_start_x = clamp_int(player_chunk_start_x, 0, NUM_CHUNK_DIVISIONS_X - 1);
  player_chunk_end_x   = clamp_int(player_chunk_end_x, 0, NUM_CHUNK_DIVISIONS_X - 1);

  chunk_sort_key_t chunk_sort_keys[NUM_CHUNK_DIVISIONS_Y * NUM_CHUNK_DIVISIONS_X * NUM_CHUNK_DIVISIONS_Z];
  int num_visible_chunks = 0;

  /* collect visible chunks */
  for(int y_plane = player_chunk_start_y; y_plane <= player_chunk_end_y; y_plane++){
    for(int z_row = player_chunk_start_z; z_row <= player_chunk_end_z; z_row++){
      for(int x_col = player_chunk_start_x; x_col <= player_chunk_end_x; x_col++){
        chunk_t *chunk = &chunks[y_plane][z_row][x_col];

        chunk_intersection_mode_t intersection_mode = get_chunk_intersection_mode_fast(chunk);

        if(intersection_mode != CHUNK_INTERSECTION_MODE_INSIDE && intersection_mode != CHUNK_INTERSECTION_MODE_INTERSECTING) continue;

        /* compute chunk center */
        float center_x = (float)((chunk->x_min + chunk->x_max) * 0.5);
        float center_y = (float)((chunk->y_min + chunk->y_max) * 0.5);
        float center_z = (float)((chunk->z_min + chunk->z_max) * 0.5);

        vector4_t chunk_center_view = matrix4_mul_vector3(&view_matrix, (vector3_t){center_x, center_y, center_z});

        chunk_sort_keys[num_visible_chunks].chunk_y_plane = y_plane;
        chunk_sort_keys[num_visible_chunks].chunk_z_row   = z_row;
        chunk_sort_keys[num_visible_chunks].chunk_x_col   = x_col;
        chunk_sort_keys[num_visible_chunks].sort_z        = chunk_center_view.z;

        num_visible_chunks++;
      }
    }
  }

  /* insertion sort (front-to-back = nearest first) */
  for(int i = 1; i < num_visible_chunks; i++){
    chunk_sort_key_t key = chunk_sort_keys[i];
    int j = i - 1;

    while(j >= 0 && chunk_sort_keys[j].sort_z > key.sort_z){
      chunk_sort_keys[j + 1] = chunk_sort_keys[j];
      j--;
    }

    chunk_sort_keys[j + 1] = key;
  }

  /* render in sorted order */
  for(int i = 0; i < num_visible_chunks; i++){
    int y_plane = chunk_sort_keys[i].chunk_y_plane;
    int z_row   = chunk_sort_keys[i].chunk_z_row;
    int x_col   = chunk_sort_keys[i].chunk_x_col;

    //render_chunk_objects(&chunks[z_row][x_col]);
    render_chunk_blocks(y_plane, z_row, x_col);

    total_chunks_rendered++;
  }

  if(options.render_chunk_outlines) draw_chunk_outlines();
  if(options.render_collision_aabb_outlines) draw_collision_aabb_outlines();
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

void render_block_quad(block_quad_t block_quad, block_material_t *block_material, bool looked_at, block_orientation_t block_orientation){
  /* triangles list for clipped triangles */
  triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES - 2]; /* for a polygon with N vertices, the number of nested triangles is always N-2 */
  vector4_t transformed_vertices[4];

  transformed_vertices[0] = matrix4_mul_vector3(&view_matrix, block_quad.points[0]); /* affine transforms so can use this faster multiply */
  transformed_vertices[1] = matrix4_mul_vector3(&view_matrix, block_quad.points[1]); 
  transformed_vertices[2] = matrix4_mul_vector3(&view_matrix, block_quad.points[2]); 
  transformed_vertices[3] = matrix4_mul_vector3(&view_matrix, block_quad.points[3]); 

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
  if(block_material->backface_culling){
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
    block_quad.uv[0],
    block_quad.uv[1],
    block_quad.uv[2],
    triangles_after_clipping
  );
  num_triangles_from_polygon_right = frustrum_clip_triangle_and_re_triangularize(
    (vector3_t){transformed_vertices[3].x, transformed_vertices[3].y, transformed_vertices[3].z},
    (vector3_t){transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z},
    (vector3_t){transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z},
    block_quad.uv[3],
    block_quad.uv[4],
    block_quad.uv[5],
    triangles_after_clipping + num_triangles_from_polygon_left
  );

  total_num_triangles_from_polygon = num_triangles_from_polygon_left + num_triangles_from_polygon_right;

  /* LIGHT CALCULATIONS */
  /*
    find light intensity to face based on dot product between face orthogonal and light direction
    result needs minus sign because the light direction vector is written as the opposite of the actual light direction we need to see (i.e y = 1 would mean shining up, but we actually want shine from above for the case y = 1)
    also, most importantly, because all vertices are transformed by the view_matrix, the light direction vector also needs to be transformed
    this is because a transformation by view_matrix amounts to a rotation(plus translation) of all vertices, which changes the face normals
    ** BUT changing normals due to camera transforms should not affect how the light behaves, and so the light direction vector must track any rotation.
    the view_matrix translation part should be ignored, and hence why the w component of vector in the multiplication below is be 0 to nulify any translations.
    ** BUT for point light its the opposite and w must be 1 because point light DOES have a position and that position must also be transformed by the view matrix.

    light_intensities field is initialized to zero below in declaration - so that the light_intensities field is all 0 in case object not reflects_light
  */
  vector3_t v_face_centroid = v_a;
  float light_intensities[MAX_NUM_LIGHT_SOURCES] = {0.0f}; // keeps the total light intensity for this face
  if(block_material->reflects_light){
    vector3_normalize_fast_approx(&v_face_normal); /* normalize here for use below */
    for(uint32_t light_index = 0; light_index < num_light_sources; light_index++){
      switch(light_sources[light_index].type){
        case LIGHT_DIRECTIONAL:{
          vector4_t light_direction_view = matrix4_mul_vector4(&view_matrix, (vector4_t){light_sources[light_index].direction.x, light_sources[light_index].direction.y, light_sources[light_index].direction.z, 0});
          /* if not translucent, then face only lights up if sgn(cam_ray dot normal) == sgn (light_direction dot normal) */
          vector3_t cam_ray = {.x = 0 - v_face_centroid.x, .y = 0 - v_face_centroid.y, .z = 0 - v_face_centroid.z}; 
          float camera_v_face_centroid_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
          float light_direction_dot_normal  = -(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
          if((light_direction_dot_normal > 0.0f && camera_v_face_centroid_dot_normal > 0.0f) || (light_direction_dot_normal < 0.0f && camera_v_face_centroid_dot_normal < 0.0f)){
            light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
          }
          else light_intensities[light_index] = 0.0f;
          light_intensities[light_index] *= light_sources[light_index].intensity;
          break;
        }

        case LIGHT_POINT:{
          vector4_t light_position_view = matrix4_mul_vector3(&view_matrix, (vector3_t){light_sources[light_index].position.x, light_sources[light_index].position.y, light_sources[light_index].position.z});
          vector3_t lightpos_to_v_face_centroid_ray = (vector3_t){light_position_view.x - v_face_centroid.x, light_position_view.y - v_face_centroid.y, light_position_view.z - v_face_centroid.z};
          float lightpos_to_v_face_centroid_ray_length = vector3_length_fast_approx(lightpos_to_v_face_centroid_ray);
          
          vector3_t cam_ray = {.x = 0 - v_face_centroid.x, .y = 0 - v_face_centroid.y, .z = 0 - v_face_centroid.z}; 
          float light_v_face_centroid_dot_normal  = v_face_normal.x * lightpos_to_v_face_centroid_ray.x + v_face_normal.y * lightpos_to_v_face_centroid_ray.y + v_face_normal.z * lightpos_to_v_face_centroid_ray.z;
          float camera_v_face_centroid_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
          if((light_v_face_centroid_dot_normal > 0.0f && camera_v_face_centroid_dot_normal > 0.0f) || (light_v_face_centroid_dot_normal < 0.0f && camera_v_face_centroid_dot_normal < 0.0f)){
            light_intensities[light_index] = fabsf(light_v_face_centroid_dot_normal) / lightpos_to_v_face_centroid_ray_length;
            light_intensities[light_index] /= 1.0f + lightpos_to_v_face_centroid_ray_length;
            light_intensities[light_index] *= light_sources[light_index].intensity;
          }
          else
            light_intensities[light_index] = 0.0f;
          break;
        }

        case LIGHT_AMBIENT:{
          light_intensities[light_index] = light_sources[light_index].intensity;
          break;
        }
      }

      // clamp to [0.0, 1.0]
      if(light_intensities[light_index] < 0.0f) light_intensities[light_index] = 0.0f;
      else if(light_intensities[light_index] > 1.0f) light_intensities[light_index] = 1.0f;
    }
  }

  // find total light intensity on this face - this is total intensity per rgb channel
  light_intensity_rgb_t total_light_intensity = find_total_light_intensity(light_intensities);

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
    
    // select texture depending on which face this is
    texture_t *quad_texture;
    switch(block_quad.quad_type){
      case VOXEL_QUAD_TOP:
        quad_texture = block_material->top_texture;
        break;
      case VOXEL_QUAD_BOTTOM:
        quad_texture = block_material->bottom_texture;
        break;
      case VOXEL_QUAD_FRONT:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: quad_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_BACK:  quad_texture = block_material->back_texture; break;
          case BLOCK_ORIENTATION_LEFT:  quad_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: quad_texture = block_material->right_texture; break;
        }
        break;
      case VOXEL_QUAD_BACK:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: quad_texture = block_material->back_texture; break;
          case BLOCK_ORIENTATION_BACK:  quad_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_LEFT:  quad_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: quad_texture = block_material->right_texture; break;
        }
        break;
      case VOXEL_QUAD_LEFT:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: quad_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_BACK:  quad_texture = block_material->right_texture; break;
          case BLOCK_ORIENTATION_LEFT:  quad_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: quad_texture = block_material->front_texture; break;
        }
        break;
      case VOXEL_QUAD_RIGHT:
      default:
        switch(block_orientation){
          case BLOCK_ORIENTATION_FRONT: quad_texture = block_material->right_texture; break;
          case BLOCK_ORIENTATION_BACK:  quad_texture = block_material->left_texture; break;
          case BLOCK_ORIENTATION_LEFT:  quad_texture = block_material->front_texture; break;
          case BLOCK_ORIENTATION_RIGHT: default: quad_texture = block_material->front_texture; break;
        }
        break;
    }

    //projected_triangle.avg_w = (projected_triangle.points[0].w + projected_triangle.points[1].w + projected_triangle.points[2].w) / 3.0f;

    projected_triangle.face_material = (material_t){
      .material_name      = "",
      .alpha_cutoff       = block_material->alpha_cutoff,
      .base_color         = block_material->base_color,
      .blend_mode         = block_material->blend_mode,
      .dither             = block_material->dither,
      .emissive_color     = block_material->emissive_color,
      .opacity            = block_material->opacity,
      .reflects_light     = block_material->reflects_light,
      .two_sided_lighting = false,
      .texture            = quad_texture
    };
    projected_triangle.total_light_intensity = total_light_intensity;

    if(looked_at){
      int32_t red   = projected_triangle.face_material.emissive_color.rgb.red   + 100;
      int32_t green = projected_triangle.face_material.emissive_color.rgb.green + 100;
      int32_t blue  = projected_triangle.face_material.emissive_color.rgb.blue  + 100;

      if(red   > 255) red   = 255;
      if(green > 255) green = 255;
      if(blue  > 255) blue  = 255;
      projected_triangle.face_material.emissive_color.rgb.red   = red;
      projected_triangle.face_material.emissive_color.rgb.green = green;
      projected_triangle.face_material.emissive_color.rgb.blue  = blue;
    }

    /* TRIANGLE RASTERIZATION */
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

/*
  loop through all blocks inside chunk
  if air, ignore
  if land, then form some or all of the block faces based on the nearest neighbours:
    if the immediate neighbour of a face is solid, then do not form that block's face etc
    if immediate neighbour is air, then form that quad with 2 faces and render
*/
void render_chunk_blocks(int32_t y_plane, int32_t z_row, int32_t x_col){
  total_chunk_triangles_to_render = 0;

  chunk_t *chunk = &chunks[y_plane][z_row][x_col];
  
  // find the start and end block indices inside chunk
  int32_t y_plane_start = chunk->y_plane_start;
  int32_t y_plane_end   = chunk->y_plane_end;
  int32_t z_row_start   = chunk->z_row_start;
  int32_t z_row_end     = chunk->z_row_end;
  int32_t x_col_start   = chunk->x_col_start;
  int32_t x_col_end     = chunk->x_col_end;

  // loop through all blocks in chunk
  for(int32_t y_plane = y_plane_start; y_plane <= y_plane_end; y_plane++){
    for(int32_t z_row = z_row_start; z_row <= z_row_end; z_row++){
      for(int32_t x_col = x_col_start; x_col <= x_col_end; x_col++){
        
        if(block_at(y_plane, z_row, x_col).solid == false) continue;
        
        int32_t left_neighbour_index    = x_col - 1;
        int32_t right_neighbour_index   = x_col + 1;
        int32_t top_neighbour_index     = y_plane + 1; 
        int32_t bottom_neighbour_index  = y_plane - 1;
        int32_t front_neighbour_index   = z_row - 1;
        int32_t back_neighbour_index    = z_row + 1;

        // a quad is rendered only if its adjescent block is AIR or if it is transparent (using alpha test)
        bool render_left_quad    = (left_neighbour_index < 0)                  ? false : (block_at(y_plane, z_row, left_neighbour_index).type  == BLOCK_TYPE_AIR || block_at(y_plane, z_row, left_neighbour_index).block_material->blend_mode == BLEND_MODE_ALPHA_TEST );
        bool render_right_quad   = (right_neighbour_index == MAP_NUM_VOXELS_X) ? false : (block_at(y_plane, z_row, right_neighbour_index).type == BLOCK_TYPE_AIR || block_at(y_plane, z_row, right_neighbour_index).block_material->blend_mode == BLEND_MODE_ALPHA_TEST);
        bool render_top_quad     = (top_neighbour_index == MAP_NUM_VOXELS_Y)   ? false : (block_at(top_neighbour_index, z_row, x_col).type     == BLOCK_TYPE_AIR || block_at(top_neighbour_index, z_row, x_col).block_material->blend_mode == BLEND_MODE_ALPHA_TEST    );
        bool render_bottom_quad  = (bottom_neighbour_index < 0)                ? false : (block_at(bottom_neighbour_index, z_row, x_col).type  == BLOCK_TYPE_AIR || block_at(bottom_neighbour_index, z_row, x_col).block_material->blend_mode == BLEND_MODE_ALPHA_TEST );
        bool render_front_quad   = (front_neighbour_index < 0)                 ? false : (block_at(y_plane, front_neighbour_index, x_col).type == BLOCK_TYPE_AIR || block_at(y_plane, front_neighbour_index, x_col).block_material->blend_mode == BLEND_MODE_ALPHA_TEST );
        bool render_back_quad    = (back_neighbour_index == MAP_NUM_VOXELS_Z)  ? false : (block_at(y_plane, back_neighbour_index, x_col).type  == BLOCK_TYPE_AIR || block_at(y_plane, back_neighbour_index, x_col).block_material->blend_mode == BLEND_MODE_ALPHA_TEST);

        block_material_t *block_material = block_at(y_plane, z_row, x_col).block_material;

        bool block_looked_at_left   = false;
        bool block_looked_at_right  = false;
        bool block_looked_at_top    = false;
        bool block_looked_at_bottom = false;
        bool block_looked_at_front  = false;
        bool block_looked_at_back   = false;
        if(looked_at_block.hit){
          if(y_plane == looked_at_block.block_y && z_row == looked_at_block.block_z && x_col == looked_at_block.block_x){
            if(looked_at_block.normal_x == -1) block_looked_at_left = true;
            else if(looked_at_block.normal_x ==  1) block_looked_at_right = true;
            else if(looked_at_block.normal_y == -1) block_looked_at_bottom = true;
            else if(looked_at_block.normal_y ==  1) block_looked_at_top = true;
            else if(looked_at_block.normal_z == -1) block_looked_at_front = true;
            else if(looked_at_block.normal_z ==  1) block_looked_at_back = true;
          }
        }

        block_orientation_t block_orientation = block_at(y_plane, z_row, x_col).orientation;
        block_quad_t block_quad;
        if(render_left_quad){
          block_quad = generate_block_quad(VOXEL_QUAD_LEFT, y_plane, z_row, x_col);
          render_block_quad(block_quad, block_material, block_looked_at_left, block_orientation);
        }
        if(render_right_quad){
          block_quad = generate_block_quad(VOXEL_QUAD_RIGHT, y_plane, z_row, x_col);
          render_block_quad(block_quad, block_material, block_looked_at_right, block_orientation);
        }
        if(render_top_quad){
          block_quad = generate_block_quad(VOXEL_QUAD_TOP, y_plane, z_row, x_col);
          render_block_quad(block_quad, block_material, block_looked_at_top, block_orientation);
        }
        if(render_bottom_quad){
          block_quad = generate_block_quad(VOXEL_QUAD_BOTTOM, y_plane, z_row, x_col);
          render_block_quad(block_quad, block_material, block_looked_at_bottom, block_orientation);
        }
        if(render_front_quad){
          block_quad = generate_block_quad(VOXEL_QUAD_FRONT, y_plane, z_row, x_col);
          render_block_quad(block_quad, block_material, block_looked_at_front, block_orientation);
        }
        if(render_back_quad){
          block_quad = generate_block_quad(VOXEL_QUAD_BACK, y_plane, z_row, x_col);
          render_block_quad(block_quad, block_material, block_looked_at_back, block_orientation);
        }
      }
    }
  }
}

void render_object(object_t *object){
  /* triangles list for clipped triangles */
  triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES - 2]; /* for a polygon with N vertices, the number of nested triangles is always N-2 */

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
    matrix4_t world_matrix      = matrix4_gen_world_matrix(object);
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
    //vector3_t v_face_centroid = {(v_a.x + v_b.x + v_c.x) / 3.0f, (v_a.y + v_b.y + v_c.y) / 3.0f, (v_a.z + v_b.z + v_c.z) / 3.0f};
    vector3_t v_face_centroid = v_a;
    float light_intensities[MAX_NUM_LIGHT_SOURCES] = {0.0f}; // keeps the total light intensity for this face
    if(face_material.reflects_light){
      vector3_normalize_fast_approx(&v_face_normal); /* normalize here for use below */
      for(uint32_t light_index = 0; light_index < num_light_sources; light_index++){
        if(light_sources[light_index].type == LIGHT_DIRECTIONAL){
          vector4_t light_direction_view = matrix4_mul_vector4(&view_matrix, (vector4_t){light_sources[light_index].direction.x, light_sources[light_index].direction.y, light_sources[light_index].direction.z, 0});
          if(face_material.two_sided_lighting)
            light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
          else{
            /* if not translucent, then face only lights up if sgn(cam_ray dot normal) == sgn (light_direction dot normal) */
            vector3_t cam_ray = {.x = 0 - v_face_centroid.x, .y = 0 - v_face_centroid.y, .z = 0 - v_face_centroid.z}; 
            float camera_v_face_centroid_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
            float light_direction_dot_normal  = -(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
            if((light_direction_dot_normal > 0.0f && camera_v_face_centroid_dot_normal > 0.0f) || (light_direction_dot_normal < 0.0f && camera_v_face_centroid_dot_normal < 0.0f)){
              light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
            }
            else light_intensities[light_index] = 0.0f;
          }
          light_intensities[light_index] *= light_sources[light_index].intensity;
        }
        else if(light_sources[light_index].type == LIGHT_POINT){
          vector4_t light_position_view = matrix4_mul_vector3(&view_matrix, (vector3_t){light_sources[light_index].position.x, light_sources[light_index].position.y, light_sources[light_index].position.z});
          vector3_t lightpos_to_v_face_centroid_ray = (vector3_t){light_position_view.x - v_face_centroid.x, light_position_view.y - v_face_centroid.y, light_position_view.z - v_face_centroid.z};
          float lightpos_to_v_face_centroid_ray_length = vector3_length_fast_approx(lightpos_to_v_face_centroid_ray);
          
          if(face_material.two_sided_lighting){
            light_intensities[light_index] = fabsf(v_face_normal.x * lightpos_to_v_face_centroid_ray.x + v_face_normal.y * lightpos_to_v_face_centroid_ray.y + v_face_normal.z * lightpos_to_v_face_centroid_ray.z) / (lightpos_to_v_face_centroid_ray_length);
            light_intensities[light_index] /= 1.0f + lightpos_to_v_face_centroid_ray_length;
            light_intensities[light_index] *= light_sources[light_index].intensity;
          }
          else{
            vector3_t cam_ray = {.x = 0 - v_face_centroid.x, .y = 0 - v_face_centroid.y, .z = 0 - v_face_centroid.z}; 
            float light_v_face_centroid_dot_normal  = v_face_normal.x * lightpos_to_v_face_centroid_ray.x + v_face_normal.y * lightpos_to_v_face_centroid_ray.y + v_face_normal.z * lightpos_to_v_face_centroid_ray.z;
            float camera_v_face_centroid_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
            if((light_v_face_centroid_dot_normal > 0.0f && camera_v_face_centroid_dot_normal > 0.0f) || (light_v_face_centroid_dot_normal < 0.0f && camera_v_face_centroid_dot_normal < 0.0f)){
              light_intensities[light_index] = fabsf(light_v_face_centroid_dot_normal) / lightpos_to_v_face_centroid_ray_length;
              light_intensities[light_index] /= 1.0f + lightpos_to_v_face_centroid_ray_length;
              light_intensities[light_index] *= light_sources[light_index].intensity;
            }
            else
              light_intensities[light_index] = 0.0f;
          } 
        }
        else if(light_sources[light_index].type == LIGHT_AMBIENT){
          light_intensities[light_index] = light_sources[light_index].intensity;
        }
        /* clamp to [0.0, 1.0] */
        if(light_intensities[light_index] < 0.0f) light_intensities[light_index] = 0.0f;
        else if(light_intensities[light_index] > 1.0f) light_intensities[light_index] = 1.0f;
      }
    }

    // find total light intensity on this face - this is total intensity per rgb channel
    light_intensity_rgb_t total_light_intensity = find_total_light_intensity(light_intensities);

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
  float sin_eta;

  /* skybox is a special case and needs to always track camera position */
  if(skybox_id != -1){
    object_t *skybox = &objects[skybox_id];

    sun_eta += M_2_PI * 1.0f / DAY_LENGTH;
    sin_eta  = sinf(sun_eta);

    sun_direction.x = 0.0f;
    sun_direction.y = sin_eta;
    sun_direction.z = cosf(sun_eta);

    if(sin_eta < 0.0f) sin_eta = 0.0f;

    light_sources[1].direction = (vector3_t){sun_direction.x, -sun_direction.y, -sun_direction.z};

    skybox->translation.x  = player.position.x; 
    skybox->translation.y  = player.position.y;
    skybox->translation.z  = player.position.z;

    skybox->mesh->materials[0].emissive_color.rgb.red   = light_sources[1].color.rgb.red   = 200 +  55 * (1.0f - sin_eta);
    skybox->mesh->materials[0].emissive_color.rgb.green = light_sources[1].color.rgb.green = 100 + 155 * sin_eta;
    skybox->mesh->materials[0].emissive_color.rgb.blue  = light_sources[1].color.rgb.blue  = 255       * sin_eta;

    //* obj 0 is skybox */
    float emissive_intensity = (sin_eta < MIN_SKYBOX_INTENSITY) ? MIN_SKYBOX_INTENSITY : sin_eta;
    skybox->mesh->materials[0].emissive_color.rgb.red   = 255.0f * (emissive_intensity); /* sky luminosity as a function of time */
    skybox->mesh->materials[0].emissive_color.rgb.green = 255.0f * (emissive_intensity); /* sky luminosity as a function of time */
    skybox->mesh->materials[0].emissive_color.rgb.blue  = 255.0f * (emissive_intensity); /* sky luminosity as a function of time */

    light_sources[1].intensity = sin_eta;
    /* as sun sets or rises, ambient light should get bluer, and oranger towards noon */
    light_sources[0].color = skybox->mesh->materials[0].emissive_color; 
    light_sources[0].intensity = MIN_AMBIENT_LIGHT_INTENSITY + AMBIENT_LIGHT_MAX_FACTOR * sin_eta;
  }
}

void update_state(void){

  update_light();

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

  /* camera tracks player */
  camera.position = (vector3_t){
    .x = player.position.x,
    .y = player.position.y + player.height * 0.92,
    .z = player.position.z
  };
  camera.direction = (vector3_t){
    .x = cosf(player.eta) * player.direction.x,
    .y = sinf(player.eta),
    .z = cosf(player.eta) * player.direction.z
  };

  player_block_x = player.position.x;
  player_block_y = player.position.y;
  player_block_z = player.position.z;

  player_chunk_x = player_block_x / CHUNK_SIZE;
  player_chunk_y = player_block_y / CHUNK_SIZE;
  player_chunk_z = player_block_z / CHUNK_SIZE;

  // PLAYER TOOLS
  if(mining){
    if(animation_direction == 1){
      animation_frame++;
      if(animation_frame >= 0) animation_direction = -1;
    }
    else{
      animation_frame--;
      if(animation_frame <= -3) animation_direction = 1;
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

  objects[pickaxe_id].translation.x = camera.position.x + -0.6*yu.x + ZNEAR * camera.direction.x + (mining? 0.3 : 0.05)* tool_direction.x - 0.4f * player_direction_orthogonal_left_right.x ;
  objects[pickaxe_id].translation.y = camera.position.y + -0.6*yu.y + ZNEAR * camera.direction.y + (mining? 0.3 : 0.05)* tool_direction.y;
  objects[pickaxe_id].translation.z = camera.position.z + -0.6*yu.z + ZNEAR * camera.direction.z + (mining? 0.3 : 0.05)* tool_direction.z - 0.4f * player_direction_orthogonal_left_right.z;

  // ROTATION
  objects[pickaxe_id].rotation.y  = -player.theta + M_PI_2 - 0.5;
  float animation_angle = 20*(M_PI) * (float)animation_frame / 8.0f;
  float rotation_x = -M_PI_4 - player.eta + animation_angle * delta_time;
  objects[pickaxe_id].rotation.x = rotation_x;
  //objects[pickaxe_id].rotation.z = -0.1;
}

inline uint32_t rasterize_triangle(triangle_t *triangle){
  if(options.projection_mode == TEXT_WIREFRAME){
    draw_triangle(
      triangle->points[0].x, triangle->points[0].y,
      triangle->points[1].x, triangle->points[1].y,
      triangle->points[2].x, triangle->points[2].y,
      (color_t){.uint32 = 0xFFFFFFFF}
    );
    return 0;
  }
  else if(options.projection_mode == TEXT_FILLED || triangle->face_material.texture == NULL){
    return draw_filled_triangle(
      &triangle->points[0], 
      &triangle->points[1], 
      &triangle->points[2], 
      triangle->face_material.base_color,
      triangle->total_light_intensity,
      triangle->face_material.emissive_color,
      triangle->face_material.reflects_light,
      options.dither || triangle->face_material.dither
    );
  }
  else{
    switch(triangle->face_material.blend_mode){
      case BLEND_MODE_OPAQUE:
        return draw_textured_triangle_opaque(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          triangle->face_material.texture->texture, triangle->face_material.texture->texture_width, triangle->face_material.texture->texture_height,
          triangle->total_light_intensity,
          triangle->face_material.emissive_color,
          triangle->face_material.reflects_light,
          options.dither || triangle->face_material.dither
        );

      case BLEND_MODE_ALPHA_TEST:
        return draw_textured_triangle_alpha_test(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          triangle->face_material.texture->texture, triangle->face_material.texture->texture_width, triangle->face_material.texture->texture_height,
          triangle->total_light_intensity,
          triangle->face_material.emissive_color,
          triangle->face_material.reflects_light,
          options.dither || triangle->face_material.dither,
          triangle->face_material.alpha_cutoff
        );

      case BLEND_MODE_ALPHA_BLEND:
        return draw_textured_triangle_alpha_blend(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          triangle->face_material.texture->texture, triangle->face_material.texture->texture_width, triangle->face_material.texture->texture_height,
          triangle->total_light_intensity,
          triangle->face_material.emissive_color,
          triangle->face_material.reflects_light,
          options.dither || triangle->face_material.dither,
          triangle->face_material.opacity
        );
    }
  }

  return 0;
}

inline uint32_t rasterize_triangle_no_zbuffer_write(
  triangle_t *triangle, 
  material_t face_material,
  light_intensity_rgb_t total_light_intensity
){
  if(options.projection_mode == TEXT_FILLED){
    return draw_filled_triangle_no_zbuffer_write(
      &triangle->points[0], 
      &triangle->points[1], 
      &triangle->points[2], 
      face_material.base_color,
      total_light_intensity,
      face_material.emissive_color,
      face_material.reflects_light,
      options.dither
    );
  }
  else if(options.projection_mode == TEXT_WIREFRAME){
    draw_triangle(
      triangle->points[0].x, triangle->points[0].y,
      triangle->points[1].x, triangle->points[1].y,
      triangle->points[2].x, triangle->points[2].y,
      (color_t){.uint32 = 0xFFFFFFFF}
    );
    return 0;
  }
  else{
    switch(face_material.blend_mode){
      case BLEND_MODE_OPAQUE:
        return draw_textured_triangle_opaque_no_zbuffer_write(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          face_material.texture->texture, face_material.texture->texture_width, face_material.texture->texture_height,
          total_light_intensity,
          face_material.emissive_color,
          face_material.reflects_light,
          options.dither
        );

      case BLEND_MODE_ALPHA_TEST:
        return draw_textured_triangle_alpha_test_no_zbuffer_write(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          face_material.texture->texture, face_material.texture->texture_width, face_material.texture->texture_height,
          total_light_intensity,
          face_material.emissive_color,
          face_material.reflects_light,
          options.dither,
          face_material.alpha_cutoff
        );

      case BLEND_MODE_ALPHA_BLEND:
        return draw_textured_triangle_alpha_blend(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          face_material.texture->texture, face_material.texture->texture_width, face_material.texture->texture_height,
          total_light_intensity,
          face_material.emissive_color,
          face_material.reflects_light,
          options.dither,
          face_material.opacity
        );
    }
  }

  return 0;
}

/* 
  render the base colorbuffer on top of which everything else is painted
*/
void render_sun(void){
  float half_w = 0.5f * (float)window_width;
  float half_h = 0.5f * (float)window_height;
  float projection_plane_dist = half_h / tanf(fov_y * 0.5f);

  /* use actual sun elevation, mirrored above/below horizon */
  float sun_elev_abs   = fabsf(sun_direction.y);  /* 0 at horizon, 1 at poles */
  float horizon_factor = 1.0f - sun_elev_abs;     /* 1 at horizon, 0 at poles */
  /* phase-like color curves, but driven by mirrored elevation */
  float sun_red_shift   = 0.65f + 0.35f * horizon_factor;  /* red stays strongest near horizon */
  float sun_green_shift = 0.35f + 0.65f * sun_elev_abs;    /* green rises toward white */
  float sun_blue_shift  = 0.00f + 1.00f * sun_elev_abs;    /* blue weakest at horizon, strongest at poles */

  /* world -> camera rotation only(no translation, meaning w = 0) */
  /* NOTE: using optimized version of multiply */
  vector4_t vec4_sun_cam = matrix4_mul_vector4(&view_matrix, (vector4_t){sun_direction.x, sun_direction.y, sun_direction.z, 0.0f});
  vector3_t sun_cam = {vec4_sun_cam.x, vec4_sun_cam.y, vec4_sun_cam.z};

  for(int y = 0; y < window_height; y++){
    float screen_y_from_center = half_h - (float)y;

    for(int x = 0; x < window_width; x++){
      float screen_x_from_center = (float)x - half_w;

      /* vector for sun dot product */
      vector3_t dir_cam = {screen_x_from_center, screen_y_from_center, projection_plane_dist};
      vector3_normalize_fast_approx(&dir_cam);
      /* vector for sky band dot product only */

      /* dot product for sun, as we go from pixel to pixel, dot product to check for sun direction across entire projection plane */
      float sun_dot = dir_cam.x * sun_cam.x + dir_cam.y * sun_cam.y + dir_cam.z * sun_cam.z;
      if(sun_dot < 0.0f) sun_dot = 0.0f;
      /* dot product for sky band, only accounts for vertical direction in projection plane */
      float sun_dot_flat = dir_cam.y * sun_cam.y + dir_cam.z * sun_cam.z;
      if(sun_dot_flat < 0.0f) sun_dot_flat = 0.0f;
      
      /* shun shimmer */
      sun_dot *= (frame_id & 1) ? 0.9992f : 1.0f;

      float sun_glow = sun_dot * sun_dot;
      sun_glow *= sun_glow;
      sun_glow *= sun_glow;
      sun_glow *= sun_glow;

      float sun_core = sun_glow; /* smaller dot power for tighter core */
      sun_core *= sun_core;
      sun_core *= sun_core;

      float sun_band = sun_dot_flat * sun_dot_flat;

      color_t pixel = color_buffer[y * window_width + x];
      float red_t   = (float)(pixel.rgb.red  ) / 255.0f + sun_red_shift   * (sun_core + sun_glow);
      float green_t = (float)(pixel.rgb.green) / 255.0f + sun_green_shift * (sun_core + sun_glow); 
      float blue_t  = (float)(pixel.rgb.blue ) / 255.0f + sun_blue_shift  * (sun_core + sun_glow); 

      if(red_t   > 1.0f) red_t   = 1.0f;
      if(green_t > 1.0f) green_t = 1.0f;
      if(blue_t  > 1.0f) blue_t  = 1.0f;

      /* for final band blend, this works by only blending where the band is strongest 
         so instead of using constant factors for the blend, we use the strength of the band as a factor,
         and this is better because it doesn't washout the sky everywhere, but only where the band is strong. */
      float band_horizon_factor = 2.0f;
      float band_mix = 0.15f * sun_band * band_horizon_factor;
      if(band_mix > 1.0f) band_mix = 1.0f;

      red_t   = (1.0f - band_mix) * red_t   + band_mix * sun_red_shift;
      green_t = (1.0f - band_mix) * green_t + band_mix * sun_green_shift;
      blue_t  = (1.0f - band_mix) * blue_t  + band_mix * sun_blue_shift;

      color_t final_color = {
        .rgb.alpha = 0xFF, 
        .rgb.blue  = blue_t * 255.0f, 
        .rgb.green = green_t * 255.0f, 
        .rgb.red   = red_t * 255.0f
      };
      
      color_buffer[y * window_width + x] = final_color;
    }
  }
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
      snprintf(debug_screen_data, 1024, 
        "on ground: %s, velocity: %.2f:%.2f:%.2f\n"
        "%ut, %upx, %uc, %ufps\n"
        "block coords: %d:%d:%d, chunk coords: %d:%d\n"
        "player pos: %.2f:%.2f:%.2f, look direction: %.2f:%.2f:%.2f, theta/eta: %.2f:%.2f\n",
        player.on_ground ? "yes" : "no",
        player.velocity.x, player.velocity.y, player.velocity.z,
        total_triangles_rendered, total_pixels_rendered, total_chunks_rendered, fps, 
        player_block_x, player_block_y, player_block_z,
        player_chunk_x, player_chunk_z,
        player.position.x,  player.position.y,  player.position.z,
        player.direction.x, player.direction.y, player.direction.z,
        player.theta, player.eta
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
  if(mining && looked_at_block.hit){
    int32_t block_y = looked_at_block.block_y;
    int32_t block_z = looked_at_block.block_z;
    int32_t block_x = looked_at_block.block_x;

    if(block_at(block_y, block_z, block_x).mineable){
      block_at(block_y, block_z, block_x).mining_amount--;
      if(options.instant_mining) block_at(block_y, block_z, block_x).mining_amount = 0;
      if(block_at(block_y, block_z, block_x).mining_amount == 0){
        player_obtain_block(block_at(block_y, block_z, block_x).type);
        assign_block_material(block_y, block_z, block_x, BLOCK_TYPE_AIR);
        block_at(block_y, block_z, block_x).solid = false;
      }
    }
  }
}

void render_objects(){
  for(uint32_t object_i = 0; object_i < num_objects; object_i++){
    render_object(&objects[object_i]);
  }
}

int main(int argc, char *argv[]){
  (void)argc;
  (void)argv;

  quit = !initialize_window();
  setup();
  
  for(; !quit; ){
    process_frame_rate();
    process_input();

    if(!console.is_open){
      update_state();
      calculate_currently_looked_at_block();
      move_player();
      if(options.chunk_sort) 
        render_visible_chunks_sorted();
      else 
        render_visible_chunks();

      render_objects();
      process_mining();

      print_debug_data();
      render_ui();

    }
    else{
      render_console();
    }

    sdl_render();
    frame_id++; /* one count per frame */
  }

  destroy_window();
  free_resources();

  //debug_print_allocated();

  return 0;
}

void load_skybox(){
  mesh_t *mesh;

  mesh = load_mesh("./assets/obj/skybox_cube.obj", false);
  skybox_id = create_object(mesh, OBJECT_TYPE_SKYBOX, false, false, false, 
                           (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f}, (vector3_t){0.0f, 0.0f, 0.0f},
                           (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
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
