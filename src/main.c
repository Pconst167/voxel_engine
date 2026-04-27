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
      if camera is on same side as light, then we want the light to shine on the face regardeless of the face normal's direction.
      hence we test: sgn((camera - v_a) dot normal) == sgn((light - v_a) dot normal)
      and the final dot product intensity needs to be absolute value after that
      (NEED TO DO SAME TO DIRECTIONAL LIGHTS)

      mesh ordering front to back for helping with zbuffer

      for some objects we dont want the light to shine on just one side (when both camera and normal are on same side), such as trees,
      because then it looks wrong. so need to add a parameter for "two-sided-lighting"

      check for meshes totally outside frustrum and cull

      textures should be bound to objects not meshes

      for each chunk, for each object, mark a face as fully inside chunk, or not fully inside so we can skip clipping
      OR: still only TEST for clipping, but dont clip directly. if triangle crosses frustrum boundary then clip.
      then for fully inside, we only test for clipping, but for intersecting chunks, clip all triangles directly.

    TODO:
      add functions to add cube, sphere, cylinder, etc.

      convert other raster functions to fixed point

      for options such as adding culling etc, for objects or anything, use bit fields instead of bools, so I can see what the option is when looking
      at the function


      about lighting: we can interpolate the 3d x y z coordinates in a triangle per pixel, and use that to calculate the per pixel light values.
      for point light we can interpolate the real per pixel brightness with 1/r^2 law.
      but probably expensive.

      chunking etc:
        alternative: during load time, we can split triangles that cross chunks. i.e load time clipping, and separate them per chunk.

      specular light has a bug when the angle is 100% parallel to the face.  value gets unstable

      IMPLEMENT OCCLUSION CULLING: for each object, project its bounding box AABB 8 vertices to world_view->space->screen and from the 8 vertices form rectangle with min/max of x/y
      then form a grid inside rectangle. for each grid point, we replace its own depth with the nearest object depth, and test that vs zbuffer.
      only if all points are farther than zbuffer values, skip/cull object completely.
      when forming rectangle, clamp it against screen edges if it goes over.
      NOTE: after transforming to world_view, then find the nearest depth of BBOX. AFTER that we project to screen to form rectangle.
      but to find nearest depth, do not project yet.

      apply LOD? backface cull trees etc when avg_z > some distance

      specular needs to have its own intensity variable. it needs to be added to the color instead of multiplied by the pixel value. as it is it takes on the texel color and thats wrong.
      also, for two sided lighting, there is no specular, might want to add it?

      tint skydome with rising/setting sun colors on the sun side?

      create command prompt in game

      add color buffer rendering function pointers so different functions can be used/selected


      during sunset and sunrise, create a greenish blue band in the horizon.

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
#include "debug.h"

// compile time enables
#define SPECULAR_LIGHT_ENABLE 1

#define PLAYER_SPEED_SHIFT_KEY_FACTOR 2.0f
#define PLAYER_ETA_DIRECTION 1.0f /* +1: mouse back looks up, -1: mouse back looks down */
#define MOUSE_SENSITIVITY 0.005f
#define DAY_LENGTH (1.01f * 60.0f * 60.0f / 0.016f) /* hours */

#define FOV_Y_DEG 90.0
#define FOV_Y (FOV_Y_DEG * M_PI / 180.0)
#define ZNEAR 1.0f
#define ZFAR 18000.0f

/* parameters for controlling the absolute ambient light */
#define MIN_AMBIENT_LIGHT_INTENSITY 0.1f
#define AMBIENT_LIGHT_MAX_FACTOR 0.5f
#define MIN_SKYBOX_INTENSITY 0.03f
#define SPECULAR_POWER 16.0f

uint32_t frame_id = 1; /* start at 1, because all face_draw_stamp values are reset to 0 at object creation time */
char fps_str[16];
uint32_t total_triangles_rendered;
uint32_t total_chunks_rendered;
uint64_t time_count; /* keeps a count that increments every frame - used to pace events */
float time_of_day; /* keeps time the accumulation of  each delta_time per frame */
char text0[256];
char text1[256];

/* game control */
float delta_time;
float move_speed = 200.0f;
float turn_speed = 2.0f;
bool quit = false;
bool is_paused = false;

/* player data */
int player_chunk_x;
int player_chunk_z;

uint8_t color_buffer_reset = 0x00; /* repeat byte for color buffer reset */

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
vector3_t sun_direction; /* world-space sun direction vector */
float sun_eta = (M_PI_2); /* sun's eta angle, start sun on horizon */

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

  /* create objects */
  load_server_room();
  //load_voxel_world();
  //load_cave_world();
  //load_world1();

  /* normalize all light direction vectors to unity */
  initialize_lights();

  /* calculate world chunks */
  calculate_world_bounds();
  create_world_chunks();

  /* populate chunks with objects */
  populate_chunks_with_objects();

  /* populate world with aabbs */
  populate_chunks_with_collision_aabbs();

  //for(int row = 0; row < NUM_CHUNK_DIVISIONS; row++){
  //  for(int col = 0; col < NUM_CHUNK_DIVISIONS; col++){
  //    if(chunks[row][col].empty == false){
  //      printf("chunk: %d %d\n", row, col);
  //      for(uint32_t obj = 0; obj < chunks[row][col].num_chunk_objects; obj++){
  //        printf("object idx %d, object id: %d\n", obj, chunks[row][col].chunk_object_faces[obj].object_id);
  //      }
  //    }
  //  }
  //}

}

void process_input(void){
  SDL_Event event;

  while(SDL_PollEvent(&event)){
    switch(event.type){
      case SDL_QUIT:
        quit = true;
        break;

      case SDL_MOUSEMOTION:
        player.theta  -= MOUSE_SENSITIVITY                        * event.motion.xrel;
        player.eta    += MOUSE_SENSITIVITY * PLAYER_ETA_DIRECTION * event.motion.yrel;
        if(player.eta  >  ETA_MAX) player.eta =  ETA_MAX;
        if(player.eta  < -ETA_MAX) player.eta = -ETA_MAX;
        break;

      case SDL_KEYDOWN:
        if(event.key.repeat == 0){
          switch (event.key.keysym.sym){
            case SDLK_ESCAPE: 
              quit = true; 
              break;
            case SDLK_p: 
              is_paused = !is_paused; 
              break;

            case SDLK_1:{
              options.projection_mode = TEXT_PERSPECTIVE_CORRECT; 
              snprintf(text1, sizeof(char) * 256, " perspective");
              break;
            }
            case SDLK_2:{
              options.projection_mode = TEXT_AFFINE; 
              snprintf(text1, sizeof(char) * 256, " affine");
              break;
            }
            case SDLK_3:{
              options.projection_mode = TEXT_FILLED; 
              snprintf(text1, sizeof(char) * 256, " filled");
              break;
            }
            case SDLK_4:{
              options.projection_mode = TEXT_WIREFRAME; 
              snprintf(text1, sizeof(char) * 256, " wireframe");
              break;
            }

            case SDLK_5:{
              options.render_collision_aabb_outlines = !options.render_collision_aabb_outlines; 
              snprintf(text1, sizeof(char) * 256, " collision aabb outlines %s", options.render_collision_aabb_outlines ? "on" : "off");
              break;
            }
            case SDLK_6:{
              options.render_chunk_outlines = !options.render_chunk_outlines; 
              snprintf(text1, sizeof(char) * 256, " chunk outlines %s", options.render_chunk_outlines ? "on" : "off");
              break;
            }
            case SDLK_7:{
              options.backface_culling_override = !options.backface_culling_override; 
              snprintf(text1, sizeof(char) * 256, " bfc ovrd %s", options.backface_culling_override ? "on" : "off");
              break;
            }
            case SDLK_8:{
              options.backface_culling = !options.backface_culling; 
              snprintf(text1, sizeof(char) * 256, " %s", options.backface_culling ? "culled" : "not culled");
              break;
            }
            case SDLK_9:{
              options.object_render_debug = !options.object_render_debug;
              break;
            }
            case SDLK_0:{
              options.wireframe_overlay = !options.wireframe_overlay; 
              break;
            }

            case SDLK_t:{
              options.dither = !options.dither;
              break;
            }
          }
        }
    }
  }
}

void move_player(void){

  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  bool shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

  float move_speed_delta_time = ((shift)? PLAYER_SPEED_SHIFT_KEY_FACTOR * move_speed : move_speed) * delta_time;
  float turn_speed_delta_time = turn_speed * delta_time;

  /* up-down for debug */
  if(keys[SDL_SCANCODE_E])
    move_player_with_collision((vector3_t){
      .x = 0.0f, 
      .y = move_speed_delta_time, 
      .z = 0.0f
    });
  else if(keys[SDL_SCANCODE_Q])
    move_player_with_collision((vector3_t){
      .x = 0.0f, 
      .y = -move_speed_delta_time, 
      .z = 0.0f
    });

  /* turning left-right */
  if(keys[SDL_SCANCODE_LEFT])
    player.theta += turn_speed_delta_time;
  else if(keys[SDL_SCANCODE_RIGHT])
    player.theta -= turn_speed_delta_time;

  /* turning up-down */
  if(keys[SDL_SCANCODE_UP]){
    player.eta += turn_speed_delta_time;
    if(player.eta > ETA_MAX) player.eta = ETA_MAX;
  }
  else if(keys[SDL_SCANCODE_DOWN]){
    player.eta -= turn_speed_delta_time;
    if(player.eta < -ETA_MAX) player.eta = -ETA_MAX;
  }

  /* strafe left-right */
  if(keys[SDL_SCANCODE_A]){
    move_player_with_collision((vector3_t){
      .x = player_direction_orthogonal_left_right.x * move_speed_delta_time, 
      .y = 0.0f, 
      .z = player_direction_orthogonal_left_right.z * move_speed_delta_time
    });
  }
  else if(keys[SDL_SCANCODE_D]){
    move_player_with_collision((vector3_t){
      .x = -player_direction_orthogonal_left_right.x * move_speed_delta_time, 
      .y = 0.0f, 
      .z = -player_direction_orthogonal_left_right.z * move_speed_delta_time
    });
  }

  /* forwards-backwards */
  if(keys[SDL_SCANCODE_W]){
    move_player_with_collision((vector3_t){
      .x = player.direction.x * move_speed_delta_time, 
      .y = 0.0f, 
      .z = player.direction.z * move_speed_delta_time
    });
  }
  else if(keys[SDL_SCANCODE_S]){
    move_player_with_collision((vector3_t){
      .x = -player.direction.x * move_speed_delta_time, 
      .y = 0.0f, 
      .z = -player.direction.z * move_speed_delta_time
    });
  }
}

void move_player_with_collision(vector3_t move){
  player.position.x += move.x;
  player.position.z += move.z;
  resolve_player_collisions();

  player.position.y += move.y;
}

bool resolve_player_against_aabb_xz(const collision_aabb_t *box){
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
    float dist = sqrtf(dist_sq);
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

void resolve_player_collisions(){
  collision_step++;

  player_chunk_x = (int)floorf((player.position.x - world_x_min) / chunk_dx);
  player_chunk_z = (int)floorf((player.position.z - world_z_min) / chunk_dz);

  if(player_chunk_x < 0) player_chunk_x = 0;
  if(player_chunk_x >= NUM_CHUNK_DIVISIONS) player_chunk_x = NUM_CHUNK_DIVISIONS - 1;
  if(player_chunk_z < 0) player_chunk_z = 0;
  if(player_chunk_z >= NUM_CHUNK_DIVISIONS) player_chunk_z = NUM_CHUNK_DIVISIONS - 1;

  int start_x = player_chunk_x - 1;
  if(start_x < 0) start_x = 0;
  if(start_x >= NUM_CHUNK_DIVISIONS) start_x = NUM_CHUNK_DIVISIONS - 1;
  int end_x = player_chunk_x + 1;
  if(end_x < 0) end_x = 0;
  if(end_x >= NUM_CHUNK_DIVISIONS) end_x = NUM_CHUNK_DIVISIONS - 1;

  int start_z = player_chunk_z - 1;
  if(start_z < 0) start_z = 0;
  if(start_z >= NUM_CHUNK_DIVISIONS) start_z = NUM_CHUNK_DIVISIONS - 1;
  int end_z = player_chunk_z + 1;
  if(end_z < 0) end_z = 0;
  if(end_z >= NUM_CHUNK_DIVISIONS) end_z = NUM_CHUNK_DIVISIONS - 1;

  for(int chunk_z = start_z; chunk_z <= end_z; chunk_z++){
    for(int chunk_x = start_x; chunk_x <= end_x; chunk_x++){

      chunk_t *chunk = &chunks[chunk_z][chunk_x];

      for(uint32_t i = 0; i < chunk->num_collision_aabbs; i++){
        uint32_t aabb_id = chunk->collision_aabbs[i];
        collision_aabb_t *box = &collision_aabbs[aabb_id];

        if(box->collision_step == collision_step) continue;

        box->collision_step = collision_step;

        resolve_player_against_aabb_xz(box);
      }
    }
  }
}

/*
RENDER STAGE
  Render sparse/dynamic objects the normal way.
  For each non-empty chunk:
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
void render_visible_chunks(void) {
  /* reset color_buffer */
  memset(color_buffer, color_buffer_reset, total_buffer_pixels * sizeof(uint32_t));
  /* reset zbuffer - it needs to be reset here as well in case skybox object does not exist */
  memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t)); /* zbuffer is made of floats and 0x00000000 = 0.0 */

  view_matrix = matrix4_gen_view_matrix(camera.position, camera.direction, up);

  // loop through all chunks and render only ones inside or intersecting
  total_triangles_rendered = 0;
  total_chunks_rendered = 0;
  for(int z_row = 0; z_row < NUM_CHUNK_DIVISIONS; z_row++){
    for(int x_col = 0; x_col < NUM_CHUNK_DIVISIONS; x_col++){
      if(!chunks[z_row][x_col].empty){ /* if chunk is not empty, then render objects contained in chunk */
        chunk_intersection_mode_t intersection_mode = get_chunk_intersection_mode_fast(&chunks[z_row][x_col]);
        if(intersection_mode == CHUNK_INTERSECTION_MODE_INSIDE || intersection_mode == CHUNK_INTERSECTION_MODE_INTERSECTING){
          render_chunk(&chunks[z_row][x_col]);
          total_chunks_rendered++;
        }
      }
    }
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
chunk_intersection_mode_t get_chunk_intersection_mode_fast(chunk_t *chunk){
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

void render_chunk(chunk_t *chunk){
  /* triangles list for clipped triangles */
  triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES - 2]; /* for a polygon with N vertices, the number of nested triangles is always N-2 */

  /* loop through all objects in chunk */
  uint32_t num_chunk_objects = chunk->num_chunk_objects;
  for(uint32_t chunk_object_idx = 0; chunk_object_idx < num_chunk_objects; chunk_object_idx++){
    chunk_object_faces_t chunk_object_faces = chunk->chunk_object_faces[chunk_object_idx];
    object_t *object = &objects[chunk_object_faces.object_id]; /* get pointer to object in chunk */

    /* loop through all faces in the face list for object */
    uint32_t num_chunk_object_faces = chunk_object_faces.num_faces;
    for(uint32_t face_idx = 0; face_idx < num_chunk_object_faces; face_idx++){
      uint32_t face_id = chunk_object_faces.face_indices[face_idx]; /* get face id from face list */
      face_t face = object->mesh->faces[face_id];

      /* only draw a face if it has not been drawn already by any chunks */
      if(object->face_draw_stamp[face_id] == frame_id) continue;
      object->face_draw_stamp[face_id] = frame_id;

      /* WORLD MATRIX / VIEW MATRIX TRANSFORM */
      vector3_t face_vertices[3];
      vector4_t transformed_vertices[3];
      
      /* object is static, fetch object's baked transformed vertices and apply view transform only */
      face_vertices[0] = object->transformed_vertices[face.vertex_indices[0]];
      face_vertices[1] = object->transformed_vertices[face.vertex_indices[1]];
      face_vertices[2] = object->transformed_vertices[face.vertex_indices[2]];

      transformed_vertices[0] = matrix4_mul_vector3(&view_matrix, face_vertices[0]); /* affine transform so can use this faster multiply */
      transformed_vertices[1] = matrix4_mul_vector3(&view_matrix, face_vertices[1]); /* affine transform so can use this faster multiply */
      transformed_vertices[2] = matrix4_mul_vector3(&view_matrix, face_vertices[2]); /* affine transform so can use this faster multiply */

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

      /* FRUSTRUM CLIPPING */
      /* check whether face has any vertex outside frustrum, if so, apply clipping, else, render face directly */
      int num_triangles_from_polygon;
      if(chunk_any_face_vertex_outside_frustrum(transformed_vertices[0], transformed_vertices[1], transformed_vertices[2], znear, zfar)){
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
      else{
        triangles_after_clipping[0].points[0] = transformed_vertices[0];
        triangles_after_clipping[0].points[1] = transformed_vertices[1];
        triangles_after_clipping[0].points[2] = transformed_vertices[2];
        triangles_after_clipping[0].uv[0] = face.uv[0],
        triangles_after_clipping[0].uv[1] = face.uv[1];
        triangles_after_clipping[0].uv[2] = face.uv[2];
        num_triangles_from_polygon = 1;
      }

      for(int t = 0; t < num_triangles_from_polygon; t++){
        triangle_t *triangle_after_clipping = &triangles_after_clipping[t];

        /* PROJECTION MATRIX / PERSPECTIVE DIVIDE */
        triangle_t projected_triangle = {0};
        for(int j = 0; j < 3; j++){
          /* optimized multiplication - removed multiplications where matrix elements are 0 */
          vector4_t projected_vertex = matrix4_projection_mul_vector4(&projection_matrix, triangle_after_clipping->points[j]);

          /* w stores old z, so divide by w to create perspective divide */
          if(projected_vertex.w != 0.0){
            projected_vertex.x /= projected_vertex.w;
            projected_vertex.y /= projected_vertex.w;
            projected_vertex.z /= projected_vertex.w;
          }

          /* scale vertices by screen width and translate to middle of the screen */
          projected_vertex.x =  projected_vertex.x * (window_width  * 0.5) + (window_width  * 0.5);
          projected_vertex.y = -projected_vertex.y * (window_height * 0.5) + (window_height * 0.5); /* invert y coordinate to account for inverse screen y (screen y grows downward) */

          projected_triangle.points[j] = projected_vertex;
          projected_triangle.uv[j]     = triangle_after_clipping->uv[j];
        }
        
        /* LIGHT CALCULATIONS */
        /*
          find light intensity to face based on dot product between face orthogonal and light direction
          result needs minus sign because the light direction vector is written as the opposite of the actual light direction we need to see (i.e y = 1 would mean shining up, but we actually want shine from above for the case y = 1)
          also, most importantly, because all vertices are transformed by the view_matrix, the light direction vector also needs to be transformed
          this is because a transformation by view_matrix amounts to a rotation(plus translation) of all vertices, which changes the face normals
          ** BUT changing normals due to camera transforms should not affect how the light behaves, and so the light direction vector must track any rotation.
          the view_matrix translation part should be ignored, and hence why the w component of vector in the multiplication below is be 0 to nulify any translations.
          ** BUT for point light its the opposite and w must be 1 because point light DOES have a position and that position must also be transformed by the view matrix.

          if camera is on same side as light, then we want the light to shine on the face regardeless of the face normal's direction.
          hence we test: sgn((camera - v_a) dot normal) == sgn((light - v_a) dot normal) and the final dot product intensity needs to be absolute value after that
          but if object is translucent to light, then we want to light both sides of a face regardless of the camera position, or the face's normal

          light_intensities field is initialized to zero below in declaration - so that the light_intensities field is all 0 in case object not reflects_light
        */
       float light_intensities[MAX_NUM_LIGHT_SOURCES] = {0.0f}; // keeps the total light intensity for this face
        if(object->reflects_light){
          vector3_normalize_fast_approx(&v_face_normal); /* normalize here for use below */
          for(uint32_t light_index = 0; light_index < num_light_sources; light_index++){
            if(light_sources[light_index].type == LIGHT_DIRECTIONAL){
              vector4_t light_direction_view = matrix4_mul_vector4(&view_matrix, (vector4_t){light_sources[light_index].direction.x, light_sources[light_index].direction.y, light_sources[light_index].direction.z, 0});
              if(object->two_sided_lighting)
                light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
              else{
                /* if not translucent, then face only lights up if sgn(cam_ray dot normal) == sgn (light_direction dot normal) */
                vector3_t cam_ray = {.x = 0 - v_a.x, .y = 0 - v_a.y, .z = 0 - v_a.z}; 
                float camera_v_a_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
                float light_direction_dot_normal  = -(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
                if((light_direction_dot_normal > 0.0f && camera_v_a_dot_normal > 0.0f) || (light_direction_dot_normal < 0.0f && camera_v_a_dot_normal < 0.0f)){
                  light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);

                  #if SPECULAR_LIGHT_ENABLE == 1
                    if(object->specular_highlights){
                      vector3_t light_direction_view_vec3 = (vector3_t){-light_direction_view.x, -light_direction_view.y, -light_direction_view.z};
                      vector3_normalize_fast_approx(&cam_ray);
                      vector3_normalize_fast_approx(&light_direction_view_vec3);
                      vector3_t H = vector3_add(light_direction_view_vec3, cam_ray);
                      vector3_normalize_fast_approx(&H);
                      float d = fmax(vector3_dot(v_face_normal, H), 0.0f);
                      float spec = 0.0f;
                      if(d > 0.0f) spec = 5 * powf(d, SPECULAR_POWER);
                      light_intensities[light_index] += spec;
                    }
                  #endif
                }
                else light_intensities[light_index] = 0.0f;
              }
              light_intensities[light_index] *= light_sources[light_index].intensity;
            }
            else if(light_sources[light_index].type == LIGHT_POINT){
              /* the 1/r^2 relation can be done without a square root.  since r = vector length, and the relation uses r^2, we don't need to first find square root and then square it.
                we can simply find the "semi" vector length by skipping the square root (using the _squarred function).  this gives 1/r^2 without a square root.
                for dot product also, instead of finding the ray len, we use the len squared and then use fast inv square root.
              */
              vector4_t light_position_view = matrix4_mul_vector3(&view_matrix, (vector3_t){light_sources[light_index].position.x, light_sources[light_index].position.y, light_sources[light_index].position.z});
              vector3_t lightpos_to_v_a_ray = (vector3_t){light_position_view.x - v_a.x, light_position_view.y - v_a.y, light_position_view.z - v_a.z};
              float lightpos_to_v_a_ray_length_squarred = vector3_length_squarred(lightpos_to_v_a_ray);
              
              if(object->two_sided_lighting){
                light_intensities[light_index] = fabsf(v_face_normal.x * lightpos_to_v_a_ray.x + v_face_normal.y * lightpos_to_v_a_ray.y + v_face_normal.z * lightpos_to_v_a_ray.z) * q_rsqrt(lightpos_to_v_a_ray_length_squarred);
                light_intensities[light_index] /= 1.0f + lightpos_to_v_a_ray_length_squarred;
                light_intensities[light_index] *= light_sources[light_index].intensity;
              }
              else{
                vector3_t cam_ray = {.x = 0 - v_a.x, .y = 0 - v_a.y, .z = 0 - v_a.z}; 
                float light_v_a_dot_normal  = v_face_normal.x * lightpos_to_v_a_ray.x + v_face_normal.y * lightpos_to_v_a_ray.y + v_face_normal.z * lightpos_to_v_a_ray.z;
                float camera_v_a_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
                if((light_v_a_dot_normal > 0.0f && camera_v_a_dot_normal > 0.0f) || (light_v_a_dot_normal < 0.0f && camera_v_a_dot_normal < 0.0f)){
                  light_intensities[light_index] = fabsf(light_v_a_dot_normal) * q_rsqrt(lightpos_to_v_a_ray_length_squarred);

                  #if SPECULAR_LIGHT_ENABLE == 1
                    if(object->specular_highlights){
                      vector3_normalize_fast_approx(&lightpos_to_v_a_ray);
                      vector3_normalize_fast_approx(&cam_ray);
                      vector3_t H = vector3_add(lightpos_to_v_a_ray, cam_ray);
                      vector3_normalize_fast_approx(&H);
                      float d = fmax(vector3_dot(v_face_normal, H), 0.0f);
                      float spec = 0.0f;
                      if(d > 0.0f) spec = 5 * powf(d, SPECULAR_POWER);
                      light_intensities[light_index] += spec;
                    }
                  #endif
                  light_intensities[light_index] /= 1.0f + lightpos_to_v_a_ray_length_squarred;
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

        /* TRIANGLE RASTERIZATION */
        rasterize_triangle(
          &projected_triangle, 
          object->texture->texture, 
          object->texture->texture_width, object->texture->texture_height, 
          object->blend_mode, object->alpha_cutoff,
          object->base_color,
          object->opacity,
          object->reflects_light,
          object->emissive_color,
          total_light_intensity
        );

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
      } /* for(int t = 0; t < polygon.num_vertices - 2; t++) */
    } /* for(uint32_t i = 0; i < num_faces; i++) */
  } /* for(uint32_t object_i = 0; object_i < num_objects_sorted; object_i++) */
  
  if((time_count & 0x1F) == 0)
    snprintf(text0, 128, "%u, %u, %u, %d:%d, (%.2f %.2f %.2f), (%.2f %.2f %.2f), (%.2f %.2f)", 
      total_triangles_rendered, total_chunks_rendered, fps, 
      player_chunk_x, player_chunk_z,
      player.position.x, player.position.y, player.position.z,
      player.direction.x, player.direction.y, player.direction.z,
      player.theta, player.eta
    );
}

void draw_chunk_outlines(void){
  for(uint32_t row = 0; row < NUM_CHUNK_DIVISIONS; row++){
    for(uint32_t col = 0; col < NUM_CHUNK_DIVISIONS; col++){
      /* nnn = xzy */
      vector4_t p000 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_min, .y = chunks[row][col].y_min, .z = chunks[row][col].z_min});
      vector4_t p010 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_min, .y = chunks[row][col].y_min, .z = chunks[row][col].z_max});
      vector4_t p100 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_max, .y = chunks[row][col].y_min, .z = chunks[row][col].z_min});
      vector4_t p110 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_max, .y = chunks[row][col].y_min, .z = chunks[row][col].z_max});

      vector4_t p001 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_min, .y = chunks[row][col].y_max, .z = chunks[row][col].z_min});
      vector4_t p011 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_min, .y = chunks[row][col].y_max, .z = chunks[row][col].z_max});
      vector4_t p101 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_max, .y = chunks[row][col].y_max, .z = chunks[row][col].z_min});
      vector4_t p111 = matrix4_mul_vector3(&view_matrix, (vector3_t){.x = chunks[row][col].x_max, .y = chunks[row][col].y_max, .z = chunks[row][col].z_max});

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

    skybox->emissive_color.rgb.red   = light_sources[1].color.rgb.red   = 200 +  55 * (1.0f - sin_eta);
    skybox->emissive_color.rgb.green = light_sources[1].color.rgb.green = 100 + 155 * sin_eta;
    skybox->emissive_color.rgb.blue  = light_sources[1].color.rgb.blue  = 255       * sin_eta;

    //* obj 0 is skybox */
    float emissive_intensity = (sin_eta < MIN_SKYBOX_INTENSITY) ? MIN_SKYBOX_INTENSITY : sin_eta;
    skybox->emissive_color.rgb.red   = 255.0f * (emissive_intensity); /* sky luminosity as a function of time */
    skybox->emissive_color.rgb.green = 255.0f * (emissive_intensity); /* sky luminosity as a function of time */
    skybox->emissive_color.rgb.blue  = 255.0f * (emissive_intensity); /* sky luminosity as a function of time */

    light_sources[1].intensity = sin_eta;
    /* as sun sets or rises, ambient light should get bluer, and oranger towards noon */
    light_sources[0].color = skybox->emissive_color; 
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
    .y = player.position.y + player.height,
    .z = player.position.z
  };
  camera.direction = (vector3_t){
    .x = cosf(player.eta) * player.direction.x,
    .y = sinf(player.eta),
    .z = cosf(player.eta) * player.direction.z
  };
}

inline void rasterize_triangle(
  triangle_t *triangle, 
  color_t *mesh_texture, 
  uint32_t texture_width, uint32_t texture_height, 
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  color_t base_color,
  float opacity,
  bool reflects_light,
  color_t emissive_color,
  light_intensity_rgb_t total_light_intensity
){
  if(options.projection_mode == TEXT_FILLED){
    draw_filled_triangle(
      &triangle->points[0], 
      &triangle->points[1], 
      &triangle->points[2], 
      base_color,
      total_light_intensity,
      emissive_color,
      reflects_light,
      options.dither
    );
    return;
  }
  else if(options.projection_mode == TEXT_WIREFRAME){
    draw_triangle(
      triangle->points[0].x, triangle->points[0].y,
      triangle->points[1].x, triangle->points[1].y,
      triangle->points[2].x, triangle->points[2].y,
      (color_t){.uint32 = 0xFFFFFFFF}
    );
    return;
  }
  else{
    switch(blend_mode){
      case BLEND_MODE_OPAQUE:
        draw_textured_triangle_opaque(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          mesh_texture, texture_width, texture_height,
          total_light_intensity,
          emissive_color,
          reflects_light,
          options.dither
        );
        return;

      case BLEND_MODE_ALPHA_TEST:
        draw_textured_triangle_alpha_test(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          mesh_texture, texture_width, texture_height,
          total_light_intensity,
          emissive_color,
          reflects_light,
          options.dither,
          alpha_cutoff
        );
        return;

      case BLEND_MODE_ALPHA_BLEND:
        draw_textured_triangle_alpha_blend(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          mesh_texture, texture_width, texture_height,
          total_light_intensity,
          emissive_color,
          reflects_light,
          options.dither,
          opacity
        );
        return;
    }
  }
}

inline void rasterize_triangle_no_zbuffer_write(
  triangle_t *triangle, 
  color_t *texture, 
  uint32_t texture_width, uint32_t texture_height, 
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  color_t base_color,
  float opacity,
  bool reflects_light,
  color_t emissive_color,
  light_intensity_rgb_t total_light_intensity
){
  if(options.projection_mode == TEXT_FILLED){
    draw_filled_triangle_no_zbuffer_write(
      &triangle->points[0], 
      &triangle->points[1], 
      &triangle->points[2], 
      base_color,
      total_light_intensity,
      emissive_color,
      reflects_light,
      options.dither
    );
    return;
  }
  else if(options.projection_mode == TEXT_WIREFRAME){
    draw_triangle(
      triangle->points[0].x, triangle->points[0].y,
      triangle->points[1].x, triangle->points[1].y,
      triangle->points[2].x, triangle->points[2].y,
      (color_t){.uint32 = 0xFFFFFFFF}
    );
    return;
  }
  else{
    switch(blend_mode){
      case BLEND_MODE_OPAQUE:
        draw_textured_triangle_opaque_no_zbuffer_write(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          texture, texture_width, texture_height,
          total_light_intensity,
          emissive_color,
          reflects_light,
          options.dither
        );
        return;

      case BLEND_MODE_ALPHA_TEST:
        draw_textured_triangle_alpha_test_no_zbuffer_write(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          texture, texture_width, texture_height,
          total_light_intensity,
          emissive_color,
          reflects_light,
          options.dither,
          alpha_cutoff
        );
        return;

      case BLEND_MODE_ALPHA_BLEND:
        draw_textured_triangle_alpha_blend(
          &triangle->points[0], triangle->uv[0],
          &triangle->points[1], triangle->uv[1],
          &triangle->points[2], triangle->uv[2],
          texture, texture_width, texture_height,
          total_light_intensity,
          emissive_color,
          reflects_light,
          options.dither,
          opacity
        );
        return;
    }
  }
}

/* 
  render the base colorbuffer on top of which everything else is painted
*/
__attribute__((optimize("Ofast")))
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
  vector4_t vec4_sun_cam = matrix4_view_mul_vector4(&view_matrix, (vector4_t){sun_direction.x, sun_direction.y, sun_direction.z, 0.0f});
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
      sun_dot *= (time_count & 1) ? 0.9992f : 1.0f;

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

void qs_chunk(chunk_sort_key_t *array, int left, int right){
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

void qs(object_sort_key_t *array, int left, int right){
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

  if(left < j) qs(array, left, j);
  if(i < right) qs(array, i, right);
}

inline void object_quicksort(object_sort_key_t *array, int count){
  if(count > 1)
    qs(array, 0, count - 1);
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
  if((time_count & 0x1F) == 0) snprintf(fps_str, sizeof(fps_str), "fps: %d", fps);

  delta_time = frame_time / 1000.0f;
  previous_frame_time = end_time;
}

void set_title(void){
  char string[256] = {0};
  if(*text0 != '\0') strcpy(string, text0);
  if(*text1 != '\0') strcat(string, text1);

  SDL_SetWindowTitle(window, string);
}

int main(void) {

  quit = !initialize_window();
  setup();
  
  while(!quit){
    frame_id++;
    process_frame_rate();
    process_input();

    if(!is_paused){
      update_state();
      move_player();
      render_visible_chunks();
      time_count++; /* one count per frame */
    }

    sdl_render();
    set_title();
    SDL_RenderPresent(renderer);
  }

  destroy_window();
  free_resources();

  return 0;
}

void load_world1(void){
  mesh_t *mesh;
  texture_t *texture;

  create_light_source(LIGHT_AMBIENT,     (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.6f);
  create_light_source(LIGHT_DIRECTIONAL, (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f, .z = 1.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 1.0f);

  load_skybox();

  mesh = load_mesh("./assets/obj/mountain.obj");
  texture = load_texture("./assets/textures/ground/grass3.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0x00000000}, 
                0.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  mesh = load_mesh("./assets/obj/river.obj");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0x00000000},
                0.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0}, (vector3_t){-1000, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/spires.obj");
  texture = load_texture("./assets/textures/ground/mountain.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFFFF9090}, (color_t){.uint32 = 0x00000000},
               0.0f, false, true, true,
               BLEND_MODE_OPAQUE, 0x01, 
               (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0}, (vector3_t){1000, 0, 13.5}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );


  mesh = load_mesh("./assets/obj/water2.obj");
  texture = load_texture("./assets/textures/water/water.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0xFFFFFF80}, 
                0.5f, false, false, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){1, 2, 1}, (vector3_t){0, 0, 0}, (vector3_t){-1000, -50, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  
  mesh = load_mesh("./assets/obj/sphere.obj");
  texture = load_texture("./assets/textures/misc/moon.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFFA0A0}, (color_t){.uint32 = 0xFF808080},
                1.0f, false, true, false,
                BLEND_MODE_ALPHA_BLEND, 0x01, 
                (vector3_t){30, 30, 30}, (vector3_t){0, 0, 0}, (vector3_t){0, 800, -400}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0.5, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/fir.obj");
  texture = load_texture("./assets/textures/plants/branch.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFF606090}, (color_t){.uint32 = 0x00000000},
               1.0f, false, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
               (vector3_t){5, 5, 5}, (vector3_t){0, 0, 0}, (vector3_t){10,  10, 0}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
               texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFF606090}, (color_t){.uint32 = 0x00000000},
               1.0f, false, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
               (vector3_t){8, 8, 8}, (vector3_t){0, 0, 0}, (vector3_t){260,  60, 180}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFF606090}, (color_t){.uint32 = 0x00000000},
               1.0f, false, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
                (vector3_t){6, 6, 6}, (vector3_t){0, 0, 0}, (vector3_t){-60,  130, 200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFF606090}, (color_t){.uint32 = 0x00000000},
               1.0f, false, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
                (vector3_t){7, 8, 7}, (vector3_t){0, 0, 0}, (vector3_t){-160,  -15, -200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFF606090}, (color_t){.uint32 = 0x00000000},
               1.0f, false, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
                (vector3_t){6, 7, 6}, (vector3_t){0, 0, 0}, (vector3_t){-170,  -15, -50}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  
  mesh = load_mesh("./assets/obj/sakura_trunk.obj");
  texture = load_texture("./assets/textures/plants/bark2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF808090}, (color_t){.uint32 = 0x00000000},
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){1.2, 1.2, 1.2},          (vector3_t){0, 0, 0},    (vector3_t){150,  45, -60}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/sakura_leaves.obj");
  texture = load_texture("./assets/textures/plants/sakura_leaves.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
               (color_t){.uint32 = 0xFFFF60FF}, (color_t){.uint32 = 0x00000000},
               1.0f, true, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
               (vector3_t){1.2, 1.2, 1.2}, (vector3_t){0, 0, 0},    (vector3_t){150,  45, -60}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/cabin.obj");
  texture = load_texture("./assets/textures/buildings/cabin.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF8080A0}, (color_t){.uint32 = 0xFF000000},
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){1, 1, 1},    (vector3_t){0, 0, 0},    (vector3_t){-25, 9, -150}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/ufo.obj");
  texture = load_texture("./assets/textures/misc/ufo.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFFFFFF}, (color_t){.uint32 = 0xFF808080},
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0},    (vector3_t){0,  300, 200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0.8, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/baloon.obj");
  texture = load_texture("./assets/textures/misc/baloon.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF80FF}, (color_t){.uint32 = 0xFF800000},
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){2, 2, 2}, (vector3_t){0, 0, 0},    (vector3_t){150,  250, -100}, 
                (vector3_t){10, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  // 18
  mesh = load_mesh("./assets/obj/rocks/rock1.obj");
  texture = load_texture("./assets/textures/stone/rock1.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FFFF}, (color_t){.uint32 = 0xFF008080},
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){15, 15, 15}, (vector3_t){0, 0, 0},    (vector3_t){-30, 90, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0.1, 0.5, 0},
                texture
  );


  // 20
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF00AAAA},
                          1.0f, false, true, false,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){35, 35, 35}, (vector3_t){0, 0, 0},    (vector3_t){-800, 50, 20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.6, 0.2, 0},
                texture
  );

  // 21
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF0070EE},
                          1.0f, false, true, true,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){25, 35, 25}, (vector3_t){0, 0, 0},    (vector3_t){70, 150, 20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.6, 0.3, 0},
                texture
  );

  // 22
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF006020},
                          1.0f, false, true, false,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){45, 45, 45}, (vector3_t){0, 0, 0},    (vector3_t){-160, 250, 20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.7, 0.3, 0},
                texture
  );

  // 23
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFF80},
                          1.0f, false, true, false,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){45, 45, 45}, (vector3_t){0, 0, 0},    (vector3_t){-40, 360, 30}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.8, 0.5, 0},
                          texture
  );

  texture = load_texture("./assets/textures/stone/rock1.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000},
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-50, 10, -50}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/rocks/rock2.obj");
  texture = load_texture("./assets/textures/stone/rock3.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000},
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){15, 15, 15}, (vector3_t){M_PI_2, 0, 0},    (vector3_t){-80, 24, -90}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/rocks/rock2.obj");
  texture = load_texture("./assets/textures/stone/rock2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF208020},
                0.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0.2, 0, 0}, (vector3_t){1000, 148, -30}, 
                (vector3_t){0, 0, 0}, (vector3_t){0.5, 0.5, 0},
                texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000},
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-80, 18, -30}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, false, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                          (color_t){.uint32 = 0xFFFF8080}, (color_t){.uint32 = 0xFF008020},
                          1.0f, false, true, true,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-110, 120, -20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.4, 0.3, 0},
                texture
  );
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000}, 
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-80, 18, -30}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );

  mesh = load_mesh("./assets/obj/castle.obj");
  texture = load_texture("./assets/textures/walls/castle.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000},
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){10, 10, 10}, (vector3_t){0, M_PI, 0},    (vector3_t){500, 0, 600}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
}

void load_quake_world(){
  mesh_t *mesh;
  texture_t *texture;

  create_light_source(LIGHT_AMBIENT,     (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.9f);
  create_light_source(LIGHT_DIRECTIONAL, (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},   (vector3_t){.x = 0.0f, .y = -1.0f, .z = 1.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.1f);

  mesh = load_mesh("./assets/obj/quake/e4m2.obj");
  texture = load_texture("./assets/textures/walls/wall.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0x00000000},
                0.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){2, 2, 2}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
}

void load_cave_world(){
  mesh_t *mesh;
  texture_t *texture;

  create_light_source(LIGHT_AMBIENT,     (vector3_t){.x = 0.0f,     .y = 0.0f,    .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f}, (color_t){.uint32 = 0xFF4080FF}, 0.6f);
  create_light_source(LIGHT_DIRECTIONAL, (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f},   (vector3_t){.x = 0.0f, .y = -1.0f, .z = 1.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.9f);

  mesh = load_mesh("./assets/obj/cave.obj");
  texture = load_texture("./assets/textures/ground/mountain.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0x00000000},
                0.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){10, 10, 10}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
}

void load_voxel_world(){
  mesh_t *mesh;
  texture_t *texture;

  create_light_source(LIGHT_AMBIENT,     (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f,  .z = 0.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.8f);
  create_light_source(LIGHT_DIRECTIONAL, (vector3_t){.x = 0.0f,   .y = 0.0f,  .z = 0.0f},   (vector3_t){.x = 0.0f, .y = -1.0f, .z = 1.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.4f);

  mesh = load_mesh("./assets/obj/voxels.obj");
  texture = load_texture("./assets/textures/ground/mountain.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false,  (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0x00000000},
                0.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){10, 10, 10}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                texture
  );
}

void load_skybox(){
  mesh_t *mesh;
  texture_t *texture;

  mesh = load_mesh("./assets/obj/skybox_cube.obj");
  texture = load_texture("./assets/textures/sky/sky.png");
  skybox_id = create_object(mesh, OBJECT_TYPE_SKYBOX, false, false, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                           (color_t){.uint32 = 0xFF000000}, (color_t){.uint32 = 0xFF20FFFF},
                           1.0f, false, false, false, 
                           BLEND_MODE_OPAQUE, 0x01, 
                           (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );
}

void load_server_room(){
  mesh_t *mesh;
  texture_t *texture;
  int32_t object_id;

  create_light_source(LIGHT_AMBIENT, (vector3_t){.x = 0.0f,     .y = 0.0f,   .z = 0.0f},    (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFFFFFF}, 0.05f);
  create_light_source(LIGHT_POINT,   (vector3_t){.x = -200.0f,  .y = 100.0f, .z = -100.0f}, (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFF0000}, 4000.0f);
  create_light_source(LIGHT_POINT,   (vector3_t){.x = -200.0f,  .y = 100.0f, .z = 100.0f},  (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFF0000}, 4000.0f);
  create_light_source(LIGHT_POINT,   (vector3_t){.x = 200.0f,   .y = 100.0f, .z = -100.0f}, (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFF0000}, 4000.0f);
  create_light_source(LIGHT_POINT,   (vector3_t){.x = 200.0f,   .y = 100.0f, .z = 100.0f},  (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFF0000}, 4000.0f);
  create_light_source(LIGHT_POINT,   (vector3_t){.x = -260.0f,  .y = 200.0f, .z = 500.0f},  (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFFFFFF}, 4000.0f);

  /* room */
  mesh = load_mesh("./assets/obj/server_room/room.obj");
  texture = load_texture("./assets/textures/server_room/room.png");
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, true, false, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                           (color_t){.uint32 = 0xFFFFFFFF}, (color_t){.uint32 = 0x00000000},
                           0.0f, false, true, false,
                           BLEND_MODE_OPAQUE, 0x00, 
                           (vector3_t){1000, 400, 1300}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );
  float x_min  = objects[object_id].bbox_aabb.x_min;
  float x_max = objects[object_id].bbox_aabb.x_max;
  float z_min  = objects[object_id].bbox_aabb.z_min;
  float z_max = objects[object_id].bbox_aabb.z_max;
  float y_max   = objects[object_id].bbox_aabb.y_max;
  float thickness = 50;
  
  // x_min
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min - thickness / 2.0f,                   
    .x_max = x_min + thickness / 2.0f,  
    .z_min = z_min + 300, 
    .z_max = z_max, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  // x_max
  create_collision_aabb((aabb_3d_t){
    .x_min = x_max,                  
    .x_max = x_max + thickness, 
    .z_min = z_min, 
    .z_max = z_max - 300, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  // z_min
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min + 620,             
    .x_max = x_max,             
    .z_min = z_min - thickness, 
    .z_max = z_min, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min,                   
    .x_max = x_min + 600,        
    .z_min = z_min + 250, 
    .z_max = z_min + 250 + thickness, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min + 620 - thickness, 
    .x_max = x_min + 620,        
    .z_min = z_min, 
    .z_max = z_min + 300, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  // z_max                                                                                       
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min + 385,             
    .x_max = x_max,             
    .z_min = z_max - 280 - thickness, 
    .z_max = z_max - 280, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min + 430 - thickness, 
    .x_max = x_min + 430,        
    .z_min = z_max - 310, 
    .z_max = z_max, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );
  create_collision_aabb((aabb_3d_t){
    .x_min = x_min,                   
    .x_max = x_min + 420,        
    .z_min = z_max - thickness / 2.0f, 
    .z_max = z_max + thickness / 2.0f, 
    .y_min = 0.0f, 
    .y_max = y_max}, 
    object_id
  );

  /* white machine */
  mesh = load_mesh("./assets/obj/server_room/server.obj");
  texture = load_texture("./assets/textures/server_room/server.png");
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, true, true, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                           (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF202020},
                           0.0f, false, true, false,
                           BLEND_MODE_OPAQUE, 0x00,
                           (vector3_t){50, 50, 50}, (vector3_t){0, 0, 0}, (vector3_t){-8, 3, 178}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );

  /* mainframes */
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, true, true, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                           (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF202020},
                           0.0f, false, true, false,
                           BLEND_MODE_OPAQUE, 0x00, 
                           (vector3_t){50, 50, 50}, (vector3_t){0, 0, 0}, (vector3_t){45, 3, 178}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );

  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, true, true, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                           (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF202020},
                           0.0f, false, true, false,
                           BLEND_MODE_OPAQUE, 0x00, 
                           (vector3_t){50, 50, 50}, (vector3_t){0, 0, 0}, (vector3_t){100, 3, 178}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );

  mesh = load_mesh("./assets/obj/potted_tree.obj");
  texture = load_texture("./assets/textures/plants/potted_tree.png");
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, false, true, (vector3_t){.x = 0.25f, .y = 1.0f, .z = 0.25f},
                           (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000},
                           0.0f, true, true, false,
                           BLEND_MODE_ALPHA_TEST, 0x01, 
                           (vector3_t){12, 14, 12}, (vector3_t){0, 0, 0}, (vector3_t){-300, 0, 550}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );

  // drone
  mesh = load_mesh("./assets/obj/drone.obj");
  texture = load_texture("./assets/textures/misc/drone.png");
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, false, true, (vector3_t){.x = 0.5f, .y = 0.8f, .z = 0.8f},
                           (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0x00000000},
                           0.0f, true, true, false,
                           BLEND_MODE_OPAQUE, 0x00, 
                           (vector3_t){50, 50, 50}, (vector3_t){0, 0, 0}, (vector3_t){-300, 0, 250}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );

  // jupiter
  mesh = load_mesh("./assets/obj/sphere.obj");
  texture = load_texture("./assets/textures/misc/jupiter.png");
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, false, false, (vector3_t){.x = 0.5f, .y = 0.8f, .z = 0.8f},
                           (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFF8080},
                           0.0f, false, true, false,
                           BLEND_MODE_OPAQUE, 0x00, 
                           (vector3_t){15, 15, 15}, (vector3_t){M_PI_2, 0, 0}, (vector3_t){350, 100, 0}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
  );

  // cube
  mesh = create_cube_mesh((vector3_t){-1, -1, -1}, (vector3_t){1,1,1});
  texture = load_texture("./assets/textures/misc/checker.png");
  object_id = create_object(mesh, OBJECT_TYPE_NORMAL, true, true, true, (vector3_t){.x = 1.0f, .y = 1.0f, .z = 1.0f},
                           (color_t){.uint32 = 0xFFFF0000}, (color_t){.uint32 = 0xFF808080},
                           0.0f, false, true, false,
                           BLEND_MODE_OPAQUE, 0x00, 
                           (vector3_t){10, 10, 10}, (vector3_t){M_PI_2, 0, 0}, (vector3_t){-200, 100, 0}, 
                           (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0},
                           texture
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