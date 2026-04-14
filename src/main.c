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

    TODO:
      mesh ordering front to back for helping with zbuffer

      specular needs to have its own intensity variable. it needs to be added to the color instead of multiplied by the pixel value. as it is it takes on the texel color and thats wrong.
      also, for two sided lighting, there is no specular, might want to add it?

      should light intensities be inside the triangle type? or just use local array and pass onto raster?

      for some objects we dont want the light to shine on just one side (when both camera and normal are on same side), such as trees,
      because then it looks wrong. so need to add a parameter for "penetrating light"

      tint skydome with rising/setting sun colors on the sun side?

      create command prompt in game

      check for meshes totally outside frustrum and cull

      maybe use sort again to avoid drawing unseen triangles?

      add a background mesh for skybox, and set w component (after projection, before raster) to large value, and set sphere to track camera position

      add color buffer rendering function pointers so different functions can be used/selected

      textures should be bound to objects not meshes

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
     2) skybox MUST be the first object/mesh rendered.  after that we reset all zbuffer values to 0.0 so anything can be rendered on top of the skybox
*/

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <math.h>
#include "main.h"
#include "display.h"
#include "vector.h"
#include "mesh.h"
#include "triangle.h"
#include "array.h"
#include "matrix.h"
#include "light.h"
#include "misc.h"
#include "upng.h"
#include "camera.h"
#include "text.h"
#include "clipping.h"
#include "object.h"


#define CAMERA_SPEED_SHIFT_KEY_FACTOR 2.0f
#define CAMERA_ETA_DIRECTION 1.0f /* +1: mouse back looks up, -1: mouse back looks down */
#define MOUSE_SENSITIVITY 0.3f
#define DAY_LENGTH (1.01f * 60.0f * 60.0f / 0.016f) /* hours */

#define ZNEAR 0.1f
#define ZFAR  5000.0f

/* parameters for controlling the absolute ambient light */
#define MIN_AMBIENT_LIGHT_INTENSITY 0.1f
#define AMBIENT_LIGHT_MAX_FACTOR 0.5f
#define MIN_SKYBOX_INTENSITY 0.03f
#define SPECULAR_POWER 16.0f

char fps_str[16];
uint32_t total_triangles_rendered;
uint32_t time_count; /* keeps a count that increments every frame - used to pace events */
float time_of_day; /* keeps time the accumulation of  each delta_time per frame */
char text2[256];
char text3[256];

/* game control */
float delta_time;
float move_speed = 55.0f;
float turn_speed = 1.5f;
bool quit = false;
bool is_paused = false;

/* perspective and camera */
matrix4_t projection_matrix;
projection_options_t projection_options = {
  .wireframe_overlay         = false, 
  .projection_mode           = TEXT_PERSPECTIVE_CORRECT,
  .backface_culling_override = false,
  .backface_culling          = true,
  .dither                    = false,
  .object_render_debug       = false
};

float aspect_ratio_x;
float aspect_ratio_y;
float fov_y;
float fov_x;
float znear;
float zfar;
vector3_t up = {.x = 0, .y = 1, .z = 0}; /* up direction vector for view matrix */

/* sun/sky data */
object_t *skybox;
vector3_t sun_direction; /* world-space sun direction vector */
float sun_eta = (M_PI/3.0f); /* sun's eta angle, start sun on horizon */

/* objects */
object_t *ufo;
object_t *baloon;
object_t *water1;

void setup(void) {
  init_font("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20);

  aspect_ratio_x = (float)window_width  / (float)window_height;
  aspect_ratio_y = (float)window_height / (float)window_width;
  fov_y = M_PI_2;
  fov_x = 2 * atanf(tanf(fov_y / 2.0f) * aspect_ratio_x);
  znear = ZNEAR;
  zfar  = ZFAR;
  projection_matrix = matrix4_gen_perspective(fov_y, aspect_ratio_y, znear, zfar);

  /* reset zbuffer */
  memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t)); /* zbuffer is made of floats and 0x00000000 = 0.0 */

  initialize_view_frustrum_planes(fov_x, fov_y, znear, zfar);
  
  create_light_source(LIGHT_AMBIENT,     (vector3_t){.x = 0.0f,   .y = 0.0f,   .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 0.05f);
  create_light_source(LIGHT_DIRECTIONAL, (vector3_t){.x = 0.0f,   .y = 0.0f,   .z = 0.0f},   (vector3_t){.x = 0.0f, .y = 0.0f, .z = 1.0f}, (color_t){.uint32 = 0xFFFFFFFF}, 1.0f);
  create_light_source(LIGHT_POINT,       (vector3_t){.x = 0.0f,   .y = 60.0f, .z = -90.0f}, (vector3_t){.x = 0.0f, .y = 0.0f, .z = 0.0f},  (color_t){.uint32 = 0xFFFF0000}, 2000.0f);

  /* normalize all light direction vectors to unity */
  initialize_lights();

  mesh_t *mesh;
  //mesh = load_mesh("./assets/obj/quake/e1m3.obj", "./assets/textures/wall.png", BLEND_MODE_OPAQUE, 0x01);
  //create_object(mesh, OBJECT_TYPE_NORMAL, true, true, 0.8f, 1.0f, (vector3_t){3, 3, 3},       (vector3_t){0, 0, 0},    (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0});
  //return;
 
  // 0
  mesh = load_mesh("./assets/obj/skybox_cube.obj", "./assets/textures/sky/sky.png");
  skybox = create_object(mesh, OBJECT_TYPE_SKYBOX, false, 
                         (color_t){.uint32 = 0xFF000000}, (color_t){.uint32 = 0xFF2080FF}, 1.0f,
                         1.0f, false, false, false, 
                         BLEND_MODE_OPAQUE, 0x01, 
                         (vector3_t){1, 1, 1}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                         (vector3_t){0, 0, 0}, (vector3_t){0, 0.01, 0}
  );

  // 1
  mesh = load_mesh("./assets/obj/mountain.obj", "./assets/textures/ground/grass3.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                0.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){0.5, 0.5, 0.528}, (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  //2
  mesh = load_mesh("./assets/obj/spires.obj", "./assets/textures/ground/mountain.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
               (color_t){.uint32 = 0xFFFF9090}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
               0.0f, false, true, true,
               BLEND_MODE_OPAQUE, 0x01, 
               (vector3_t){0.5, 0.5, 0.5}, (vector3_t){0, 0, 0}, (vector3_t){490, 0, 0}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 3
  mesh = load_mesh("./assets/obj/river.obj", "./assets/textures/ground/mountain.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                0.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){0.5, 0.5, 0.5}, (vector3_t){0, 0, 0}, (vector3_t){-490, 0, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 4
  mesh = load_mesh("./assets/obj/water2.obj", "./assets/textures/water/water.png");
  water1 = create_object(mesh, OBJECT_TYPE_NORMAL, false, 
                (color_t){.uint32 = 0xFF80FF40}, (color_t){.uint32 = 0xFFFFFF80}, 1.0f, 
                0.5f, false, false, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){0.5, 1, 0.5}, (vector3_t){0, 0, 0}, (vector3_t){-490, -50, 0}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
  
  // 5
  mesh = load_mesh("./assets/obj/rocks/rock2.obj", "./assets/textures/stone/rock2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF40FF40}, 0.4f, 
                0.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0.2, 0, 0}, (vector3_t){1000, 148, -30}, 
                (vector3_t){0, 0, 0}, (vector3_t){0.5, 0.5, 0}
  );

  // 6
  mesh = load_mesh("./assets/obj/sphere.obj", "./assets/textures/misc/moon.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFFFFA0A0}, (color_t){.uint32 = 0xFFFFFFFF}, 0.5f,
                1.0f, false, true, false,
                BLEND_MODE_ALPHA_BLEND, 0x01, 
                (vector3_t){30, 30, 30}, (vector3_t){0, 0, 0}, (vector3_t){0, 800, -400}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0.5, 0}
  );
    
  // 7
  mesh = load_mesh("./assets/obj/tree_trunk.obj", "./assets/textures/plants/bark3.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
               (color_t){.uint32 = 0xFF606090}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
               1.0f, false, true, false,
               BLEND_MODE_OPAQUE, 0x01, 
               (vector3_t){4, 6, 4}, (vector3_t){0, 0, 0}, (vector3_t){260,  80, 180}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 8
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFF606070}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){4, 6, 4}, (vector3_t){0, 0, 0}, (vector3_t){-60,  150, 200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 9
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFF606040}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x01, 
                (vector3_t){4, 6, 4}, (vector3_t){0, 0, 0}, (vector3_t){-160,  0, -200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 10
  mesh = load_mesh("./assets/obj/tree_leaves.obj", "./assets/textures/plants/leaves2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, 
                (color_t){.uint32 = 0xFF90F030}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, true, true, false,
                BLEND_MODE_ALPHA_TEST, 0x01, 
                (vector3_t){4, 6, 4}, (vector3_t){0, 0, 0}, (vector3_t){260,  80, 180}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 11
  create_object(mesh, OBJECT_TYPE_NORMAL, false, 
                (color_t){.uint32 = 0xFFFFA030}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, true, true, false,
                BLEND_MODE_ALPHA_TEST, 0x01, 
                (vector3_t){4, 6, 4}, (vector3_t){0, 0, 0}, (vector3_t){-60,  150, 200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 12
  create_object(mesh, OBJECT_TYPE_NORMAL, false, 
                (color_t){.uint32 = 0xFF5FA020}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, true, true, false,
                BLEND_MODE_ALPHA_TEST, 0x01, 
                (vector3_t){4, 6, 4}, (vector3_t){0, 0, 0}, (vector3_t){-160,  0, -200}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
  
  // 13
  mesh = load_mesh("./assets/obj/sakura_trunk.obj", "./assets/textures/plants/bark2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFF808090}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){1.2, 1.2, 1.2},          (vector3_t){0, 0, 0},    (vector3_t){150,  45, -60}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 14
  mesh = load_mesh("./assets/obj/sakura_leaves.obj", "./assets/textures/plants/sakura_leaves.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, 
               (color_t){.uint32 = 0xFFFF60FF}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
               1.0f, true, true, false,
               BLEND_MODE_ALPHA_TEST, 0x01, 
               (vector3_t){1.2, 1.2, 1.2},          (vector3_t){0, 0, 0},    (vector3_t){150,  45, -60}, 
               (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 15
  mesh = load_mesh("./assets/obj/cabin.obj", "./assets/textures/buildings/cabin.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFF8080A0}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){1, 1, 1},    (vector3_t){0, 0, 0},    (vector3_t){-25, 10, -150}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 16
  mesh = load_mesh("./assets/obj/ufo.obj", "./assets/textures/misc/ufo.png");
  ufo = create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                      (color_t){.uint32 = 0xFFFFFFFF}, (color_t){.uint32 = 0xFFFFFFFF}, 0.5f, 
                      1.0f, false, true, true,
                      BLEND_MODE_OPAQUE, 0x00, 
                      (vector3_t){400, 400, 400}, (vector3_t){0, 0, 0},    (vector3_t){0,  300, 200}, 
                      (vector3_t){0, 0, 0}, (vector3_t){0, 0.8, 0}
  );

  // 17
  mesh = load_mesh("./assets/obj/baloon.obj", "./assets/textures/misc/baloon.png");
  baloon = create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                         (color_t){.uint32 = 0xFFFF80FF}, (color_t){.uint32 = 0xFFFF0000}, 0.5f, 
                         1.0f, false, true, true,
                         BLEND_MODE_OPAQUE, 0x00, 
                         (vector3_t){2, 2, 2}, (vector3_t){0, 0, 0},    (vector3_t){150,  250, -100}, 
                         (vector3_t){10, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 18
  mesh = load_mesh("./assets/obj/rocks/rock1.obj", "./assets/textures/stone/rock1.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                          (color_t){.uint32 = 0xFF80FFFF}, (color_t){.uint32 = 0xFF00FFFF}, 0.5f, 
                          1.0f, false, true, true,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){15, 15, 15}, (vector3_t){0, 0, 0},    (vector3_t){-30, 90, 0}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.1, 0.5, 0}
  );

  // 19
  mesh = load_mesh("./assets/obj/rocks/rock2.obj", "./assets/textures/stone/rock2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                          (color_t){.uint32 = 0xFFFF8080}, (color_t){.uint32 = 0xFF00FF40}, 0.5f, 
                          1.0f, false, true, true,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-110, 120, -20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.4, 0.3, 0}
  );

  // 20
  mesh = load_mesh("./assets/obj/rocks/rock1.obj", "./assets/textures/stone/rock1.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF00FFFF}, 0.8f, 
                          1.0f, false, true, false,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){35, 35, 35}, (vector3_t){0, 0, 0},    (vector3_t){-800, 50, 20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.6, 0.2, 0}
  );

  // 21
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF0080FF}, 0.9f, 
                          1.0f, false, true, true,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){25, 35, 25}, (vector3_t){0, 0, 0},    (vector3_t){70, 150, 20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.6, 0.3, 0}
  );

  // 22
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFF00FF80}, 0.4f, 
                          1.0f, false, true, false,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){45, 45, 45}, (vector3_t){0, 0, 0},    (vector3_t){-160, 250, 20}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.7, 0.3, 0}
  );

  // 23
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                          (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFF80}, 0.9f, 
                          1.0f, false, true, false,
                          BLEND_MODE_OPAQUE, 0x00, 
                          (vector3_t){45, 45, 45}, (vector3_t){0, 0, 0},    (vector3_t){-40, 360, 30}, 
                          (vector3_t){0, 0, 0}, (vector3_t){0.8, 0.5, 0}
  );

  // 24
  mesh = load_mesh("./assets/obj/rocks/rock1.obj", "./assets/textures/stone/rock1.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-50, 2, -50}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 25
  mesh = load_mesh("./assets/obj/rocks/rock2.obj", "./assets/textures/stone/rock3.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){15, 15, 15}, (vector3_t){M_PI_2, 0, 0},    (vector3_t){-80, 18, -90}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 26
  mesh = load_mesh("./assets/obj/rocks/rock2.obj", "./assets/textures/stone/rock2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-80, 18, -30}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 27
  mesh = load_mesh("./assets/obj/rocks/rock2.obj", "./assets/textures/stone/rock2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, true, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f, 
                1.0f, false, true, true,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){25, 25, 25}, (vector3_t){0, 0, 0},    (vector3_t){-80, 18, -30}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  // 28
  mesh = load_mesh("./assets/obj/castle.obj", "./assets/textures/walls/castle.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f,
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){10, 10, 10}, (vector3_t){0, -M_PI_2, 0},    (vector3_t){1100, 0, 40}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );

  mesh = load_mesh("./assets/obj/volcano1.obj", "./assets/textures/volcano2.png");
  create_object(mesh, OBJECT_TYPE_NORMAL, false, 
                (color_t){.uint32 = 0xFFFF8060}, (color_t){.uint32 = 0xFFFFFFFF}, 0.0f,
                1.0f, false, true, false,
                BLEND_MODE_OPAQUE, 0x00, 
                (vector3_t){0.2, 0.3, 0.1}, (vector3_t){0, 0, 0},    (vector3_t){0, 0, 1300}, 
                (vector3_t){0, 0, 0}, (vector3_t){0, 0, 0}
  );
}

void process_input(void){
  SDL_Event event;

  const uint8_t *keys = SDL_GetKeyboardState(NULL);
  bool shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
  bool _C = keys[SDL_SCANCODE_C];

  float move_speed_delta_time = ((shift || _C )? CAMERA_SPEED_SHIFT_KEY_FACTOR * move_speed : move_speed) * delta_time;
  float _turn_speed = turn_speed * delta_time;

  while(SDL_PollEvent(&event)){
    switch(event.type){
      case SDL_QUIT:
        quit = true;
        break;

      case SDL_MOUSEMOTION:
        camera.theta  -= MOUSE_SENSITIVITY                        * event.motion.xrel * delta_time;
        camera.eta    += MOUSE_SENSITIVITY * CAMERA_ETA_DIRECTION * event.motion.yrel * delta_time;
        if(camera.eta  >  ETA_MAX) camera.eta =  ETA_MAX;
        if(camera.eta  < -ETA_MAX) camera.eta = -ETA_MAX;
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
              projection_options.projection_mode = TEXT_PERSPECTIVE_CORRECT; 
              snprintf(text2, sizeof(char) * 256, "perspective");
              break;
            }
            case SDLK_2:{
              projection_options.projection_mode = TEXT_AFFINE; 
              snprintf(text2, sizeof(char) * 256, "affine");
              break;
            }
            case SDLK_3:{
              projection_options.projection_mode = TEXT_FILLED; 
              snprintf(text2, sizeof(char) * 256, "filled");
              break;
            }
            case SDLK_4:{
              projection_options.projection_mode = TEXT_WIREFRAME; 
              snprintf(text2, sizeof(char) * 256, "wireframe");
              break;
            }

            case SDLK_7:{
              projection_options.backface_culling_override = !projection_options.backface_culling_override; 
              snprintf(text2, sizeof(char) * 256, "bfc ovrd %s", projection_options.backface_culling_override ? "on" : "off");
              break;
            }
            case SDLK_8:{
              projection_options.backface_culling = !projection_options.backface_culling; 
              snprintf(text2, sizeof(char) * 256, "%s", projection_options.backface_culling ? "culled" : "not culled");
              break;
            }
            case SDLK_9:{
              projection_options.object_render_debug = !projection_options.object_render_debug;
              break;
            }
            case SDLK_0:{
              projection_options.wireframe_overlay = !projection_options.wireframe_overlay; 
              break;
            }

            case SDLK_t:{
              projection_options.dither = !projection_options.dither;
              break;
            }
          }

        }
        break;
    }
  }

  /* turning left-right */
  if(keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_H])
    camera.theta += _turn_speed;
  if(keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_L])
    camera.theta -= _turn_speed;

  /* turning up-down */
  if(keys[SDL_SCANCODE_UP]|| keys[SDL_SCANCODE_K]){
    camera.eta += _turn_speed;
    if(camera.eta > ETA_MAX) camera.eta = ETA_MAX;
  }
  if(keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_J]){
    camera.eta -= _turn_speed;
    if(camera.eta < -ETA_MAX) camera.eta = -ETA_MAX;
  }

  /* up-down */
  if(keys[SDL_SCANCODE_A])
    camera.position.y -= move_speed_delta_time;
  if(keys[SDL_SCANCODE_D])
    camera.position.y += move_speed_delta_time;

  /* strafe left-right */
  if(keys[SDL_SCANCODE_Q]){
    camera.position.x += camera_direction_orthogonal_left_right.x * 1.5 * move_speed_delta_time;
    camera.position.y += camera_direction_orthogonal_left_right.y * 1.5 * move_speed_delta_time;
    camera.position.z += camera_direction_orthogonal_left_right.z * 1.5 * move_speed_delta_time;
  }
  if(keys[SDL_SCANCODE_E]){
    camera.position.x -= camera_direction_orthogonal_left_right.x * 1.5 * move_speed_delta_time;
    camera.position.y -= camera_direction_orthogonal_left_right.y * 1.5 * move_speed_delta_time;
    camera.position.z -= camera_direction_orthogonal_left_right.z * 1.5 * move_speed_delta_time;
  }

  /* forwards-backwards */
  if(keys[SDL_SCANCODE_W]){
    camera.position.x += camera.direction.x * move_speed_delta_time;
    camera.position.y += camera.direction.y * move_speed_delta_time;
    camera.position.z += camera.direction.z * move_speed_delta_time;
  }
  if(keys[SDL_SCANCODE_S]){
    camera.position.x -= camera.direction.x * move_speed_delta_time;
    camera.position.y -= camera.direction.y * move_speed_delta_time;
    camera.position.z -= camera.direction.z * move_speed_delta_time;
  }
}

void update_light(void){
  float sin_eta;
  
  sun_eta += M_2_PI * 1.0f / DAY_LENGTH;
  sin_eta  = sinf(sun_eta);

  sun_direction.x = 0.0f;
  sun_direction.y = sin_eta;
  sun_direction.z = cosf(sun_eta);

  light_sources[1].direction = (vector3_t){sun_direction.x, -sun_direction.y, -sun_direction.z};
  if(sin_eta < 0.0f) sin_eta = 0.0f;

  skybox->emissive_color.rgb.red = light_sources[1].color.rgb.red = 200 + 55 * (1.0f - sin_eta);
  skybox->emissive_color.rgb.green = light_sources[1].color.rgb.green = 100 + 155 * sin_eta;
  skybox->emissive_color.rgb.blue = light_sources[1].color.rgb.blue = 255 * sin_eta;
  light_sources[1].intensity = sin_eta;
  /* as sun sets or rises, ambient light should get bluer, and oranger towards noon */
  //light_sources[0].color = skybox->emissive_color; 
  light_sources[0].intensity = MIN_AMBIENT_LIGHT_INTENSITY + AMBIENT_LIGHT_MAX_FACTOR * sin_eta;

  /* obj 0 is skydome */
  skybox->emissive_intensity = sin_eta < MIN_SKYBOX_INTENSITY ? MIN_SKYBOX_INTENSITY : sin_eta; /* sky luminosity as a function of time */
}

void update_state(void){
  update_light();

  ufo->velocity.x = 25 * cosf(0.1 * time_of_day);
  ufo->velocity.z = 25 * sinf(0.1 * time_of_day);

  baloon->velocity.x = 10 * cosf(0.1f * time_of_day);

  water1->scale.y = 0.4f + 0.2f * sinf(0.5f * M_2_PI * time_of_day);
}

inline void rasterize_triangle(
  triangle_t *triangle, 
  color_t *mesh_texture, 
  uint32_t texture_width, uint32_t texture_height, 
  blend_mode_t blend_mode,
  uint8_t alpha_cutoff,
  color_t solid_color,
  float opacity,
  bool reflective,
  color_t emissive_color,
  float emissive_intensity
){
  if(projection_options.projection_mode == TEXT_FILLED){
    draw_filled_triangle(
      &(vector4_t){triangle->points[0].x, triangle->points[0].y, triangle->points[0].z, triangle->points[0].w}, 
      &(vector4_t){triangle->points[1].x, triangle->points[1].y, triangle->points[0].z, triangle->points[1].w}, 
      &(vector4_t){triangle->points[2].x, triangle->points[2].y, triangle->points[0].z, triangle->points[2].w}, 
      solid_color,
      triangle->light_intensities,
      emissive_color,
      emissive_intensity,
      reflective,
      projection_options.dither
    );
    return;
  }
  else if(projection_options.projection_mode == TEXT_WIREFRAME){
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
          &(vector4_t){triangle->points[0].x, triangle->points[0].y, triangle->points[0].z, triangle->points[0].w}, (tex2_t){triangle->uv[0].u, triangle->uv[0].v},
          &(vector4_t){triangle->points[1].x, triangle->points[1].y, triangle->points[1].z, triangle->points[1].w}, (tex2_t){triangle->uv[1].u, triangle->uv[1].v},
          &(vector4_t){triangle->points[2].x, triangle->points[2].y, triangle->points[2].z, triangle->points[2].w}, (tex2_t){triangle->uv[2].u, triangle->uv[2].v},
          mesh_texture, texture_width, texture_height,
          triangle->light_intensities,
          emissive_color,
          emissive_intensity,
          reflective,
          projection_options.dither
        );
        return;

      case BLEND_MODE_ALPHA_TEST:
        draw_textured_triangle_alpha_test(
          &(vector4_t){triangle->points[0].x, triangle->points[0].y, triangle->points[0].z, triangle->points[0].w}, (tex2_t){triangle->uv[0].u, triangle->uv[0].v},
          &(vector4_t){triangle->points[1].x, triangle->points[1].y, triangle->points[1].z, triangle->points[1].w}, (tex2_t){triangle->uv[1].u, triangle->uv[1].v},
          &(vector4_t){triangle->points[2].x, triangle->points[2].y, triangle->points[2].z, triangle->points[2].w}, (tex2_t){triangle->uv[2].u, triangle->uv[2].v},
          mesh_texture, texture_width, texture_height,
          triangle->light_intensities,
          emissive_color,
          emissive_intensity,
          reflective,
          projection_options.dither,
          alpha_cutoff
        );
        return;

      case BLEND_MODE_ALPHA_BLEND:
        draw_textured_triangle_alpha_blend(
          &(vector4_t){triangle->points[0].x, triangle->points[0].y, triangle->points[0].z, triangle->points[0].w}, (tex2_t){triangle->uv[0].u, triangle->uv[0].v},
          &(vector4_t){triangle->points[1].x, triangle->points[1].y, triangle->points[1].z, triangle->points[1].w}, (tex2_t){triangle->uv[1].u, triangle->uv[1].v},
          &(vector4_t){triangle->points[2].x, triangle->points[2].y, triangle->points[2].z, triangle->points[2].w}, (tex2_t){triangle->uv[2].u, triangle->uv[2].v},
          mesh_texture, texture_width, texture_height,
          triangle->light_intensities,
          emissive_color,
          emissive_intensity,
          reflective,
          projection_options.dither,
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

  /* generate view matrix for rotating the sun vector into camera space */
  matrix4_t view_matrix = matrix4_gen_view(camera.position, camera.direction, up);

  /* world -> camera rotation only(no translation, meaning w = 0) */
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

void object_render_debug(){
  SDL_Event event;

  sdl_render();
  render_text();
  SDL_RenderPresent(renderer);

  while(1){
    SDL_WaitEvent(&event);

    if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_9){
      projection_options.object_render_debug = !projection_options.object_render_debug;
    }

    if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_o){
      return;
    }
    
    if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE){
      exit(0);
    }

    if(!projection_options.object_render_debug) return;
  }
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

void update_and_render_color_buffer(void) {
  /* reset color_buffer */
  memset(color_buffer, 0x00, total_buffer_pixels * sizeof(uint32_t));
  /* reset zbuffer - it needs to be reset here as well in case skybox object does not exist */
  memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t)); /* zbuffer is made of floats and 0x00000000 = 0.0 */

  /* view matrix */
  /* build spherical coordinates unit vector for camera direction */
  camera.direction = (vector3_t){
    .x = cosf(camera.eta) * cosf(camera.theta),
    .y = sinf(camera.eta),
    .z = cosf(camera.eta) * sinf(camera.theta)
  };
  /* vector orthogonal to camera direction: camera.direction cross up */
  camera_direction_orthogonal_left_right = (vector3_t){
    .x = camera.direction.y * up.z - camera.direction.z * up.y,
    .y = camera.direction.z * up.x - camera.direction.x * up.z,
    .z = camera.direction.x * up.y - camera.direction.y * up.x
  };
  
  matrix4_t view_matrix = matrix4_gen_view(camera.position, camera.direction, up);

  /* for each object, apply frustrum culling to entire object : apply world_view to center coord
     if not culled, then add to objects-to-be-sorted list
     sort objects by furthest z/w value
     render list of sorted objects

    need to translate center also by velocities!
     skybox w/z key set to infinity for sorting function
  */

  object_sort_key_t objects_sorted[MAX_NUM_OBJECTS]; /* list of objects to be sorted */
  uint32_t num_objects_sorted = 1;

  for(uint32_t object_i = 1; object_i < num_objects; object_i++){
    object_t object = objects[object_i];

    object.translation.x += object.velocity.x * delta_time;
    object.translation.y += object.velocity.y * delta_time;
    object.translation.z += object.velocity.z * delta_time;

    /* generate the final combined world/view matrix */
    matrix4_t world_view_matrix = matrix4_gen_world_view(&object, &view_matrix);

    /* frustrum culling for entire objects at once */
    vector4_t bsphere_center_world_view = matrix4_mul_vector3(&world_view_matrix, (vector3_t){object.mesh->bounding_center.x, object.mesh->bounding_center.y, object.mesh->bounding_center.z}); /* affine transform so can use this faster multiply */
    float scale_max = object.scale.x;
    if(object.scale.y > scale_max) scale_max = object.scale.y;
    if(object.scale.z > scale_max) scale_max = object.scale.z;
    float bsphere_radius_scaled = object.mesh->bsphere_radius * scale_max; /* scale sphere radius according to object's scale */
    bool should_frustrum_cull_object = frustrum_cull_object(&bsphere_center_world_view, bsphere_radius_scaled, znear, zfar);
    if(should_frustrum_cull_object) continue;

    /* if not culled, then add to list */
    objects_sorted[num_objects_sorted].object_index = object_i;
    objects_sorted[num_objects_sorted].sort_z = bsphere_center_world_view.z + bsphere_radius_scaled;
    num_objects_sorted++;
  } /* for(uint32_t object_i = 0; object_i < num_objects; object_i++) */

  if(num_objects_sorted > 1)
    object_quicksort(objects_sorted + 1, num_objects_sorted - 1);

  objects_sorted[0].object_index = 0; /* set the first object to be the skybox */
  /* end ofsort */

  total_triangles_rendered = 0;
  /* transform each mesh, and rasterize */
  for(uint32_t object_i = 0; object_i < num_objects_sorted; object_i++){
    uint32_t object_index = objects_sorted[object_i].object_index;
    object_t *object = &objects[object_index];
    
    switch(object->object_type){
      case OBJECT_TYPE_NORMAL:
        object->translation.x += object->velocity.x * delta_time;
        object->translation.y += object->velocity.y * delta_time;
        object->translation.z += object->velocity.z * delta_time;
        break;
      case OBJECT_TYPE_SKYBOX:
        object->translation.x  = camera.position.x; /* skybox is a special case and needs to always track camera position */
        object->translation.y  = camera.position.y;
        object->translation.z  = camera.position.z;
        break;
    }
    object->rotation.x += object->angular_velocity.x * delta_time;
    object->rotation.y += object->angular_velocity.y * delta_time;
    object->rotation.z += object->angular_velocity.z * delta_time;

    /* generate the final combined world/view matrix */
    matrix4_t world_view_matrix = matrix4_gen_world_view(object, &view_matrix);

    uint32_t num_mesh_faces = array_length(object->mesh->faces);
    for(uint32_t i = 0; i < num_mesh_faces; i++){
      face_t mesh_face = object->mesh->faces[i];

      vector3_t face_vertices[3];
      face_vertices[0] = object->mesh->vertices[mesh_face.vertex_indices[0]];
      face_vertices[1] = object->mesh->vertices[mesh_face.vertex_indices[1]];
      face_vertices[2] = object->mesh->vertices[mesh_face.vertex_indices[2]];

      /* WORLD MATRIX / VIEW MATRIX TRANSFORM */
      vector4_t transformed_vertices[3];
      for(int j = 0; j < 3; j++)
        transformed_vertices[j] = matrix4_mul_vector3(&world_view_matrix, (vector3_t){face_vertices[j].x, face_vertices[j].y, face_vertices[j].z}); /* affine transform so can use this faster multiply */

      /* BACKFACE CULLING */
      /* calculate face normals for backface culling and lighting */
      vector3_t v_face_normal = get_triangle_face_normal(transformed_vertices);
      vector3_t v_a = {transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z};
      /* Backface culling */
      if((object->backface_culling && !projection_options.backface_culling_override) || 
         (projection_options.backface_culling_override && projection_options.backface_culling)
      ){
        /* camera - v_a ray
          after view_matrix transform, camera position is always at 0, 0, 0 */
        vector3_t cam_ray = {.x = 0 - v_a.x, .y = 0 - v_a.y, .z = 0 - v_a.z}; 
        /* calculate dot product between camera ray and orthogonal to find alignment */
        float dot_face_normal_cam_ray = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
        if(dot_face_normal_cam_ray < 0) continue;
      }

      // FRUSTRUM CLIPPING
      /* create a polygon from current triangle face */
      polygon_t polygon = create_polygon_from_triangle(
        (vector3_t){transformed_vertices[0].x, transformed_vertices[0].y, transformed_vertices[0].z},
        (vector3_t){transformed_vertices[1].x, transformed_vertices[1].y, transformed_vertices[1].z},
        (vector3_t){transformed_vertices[2].x, transformed_vertices[2].y, transformed_vertices[2].z},
        mesh_face.uv[0],
        mesh_face.uv[1],
        mesh_face.uv[2]
      );
      frustrum_clip_polygon(&polygon);

      /* break polygon apart into triangles */
      triangle_t triangles_after_clipping[MAX_NUM_POLYGON_VERTICES - 2]; /* for a polygon with N vertices, the number of nested triangles is always N-2 */
      create_triangles_from_polygon(&polygon, triangles_after_clipping);

      for(int t = 0; t < polygon.num_vertices - 2; t++){
        triangle_t *triangle_after_clipping = &triangles_after_clipping[t];

        /* PROJECTION MATRIX / PERSPECTIVE DIVIDE */
        triangle_t projected_triangle = {0};
        for(int j = 0; j < 3; j++){
          vector4_t projected_vertex = matrix4_mul_vector4_project(&projection_matrix, triangle_after_clipping->points[j]);

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
        */
        if(object->reflective){
          for(uint32_t light_index = 0; light_index < num_light_sources; light_index++){
            if(light_sources[light_index].type == LIGHT_DIRECTIONAL){
              vector4_t light_direction_view = matrix4_mul_vector4(&view_matrix, (vector4_t){light_sources[light_index].direction.x, light_sources[light_index].direction.y, light_sources[light_index].direction.z, 0});
              if(object->two_sided_lighting)
                projected_triangle.light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
              else{
                /* if not translucent, then face only lights up if sgn(cam_ray dot normal) == sgn (light_direction dot normal) */
                vector3_t cam_ray = {.x = 0 - v_a.x, .y = 0 - v_a.y, .z = 0 - v_a.z}; 
                float camera_v_a_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
                float light_direction_v_a_dot_normal  = -(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);
                if((light_direction_v_a_dot_normal > 0.0f && camera_v_a_dot_normal > 0.0f) || (light_direction_v_a_dot_normal < 0.0f && camera_v_a_dot_normal < 0.0f)){
                  projected_triangle.light_intensities[light_index] = fabsf(v_face_normal.x * light_direction_view.x + v_face_normal.y * light_direction_view.y + v_face_normal.z * light_direction_view.z);

                  if(object->specular){
                    vector3_t light_direction_view_vec3 = (vector3_t){-light_direction_view.x, -light_direction_view.y, -light_direction_view.z};
                    vector3_normalize_fast_approx(&cam_ray);
                    vector3_normalize_fast_approx(&light_direction_view_vec3);
                    vector3_t H = vector3_add(light_direction_view_vec3, cam_ray);
                    vector3_normalize_fast_approx(&H);
                    float d = fmax(vector3_dot(v_face_normal, H), 0.0f);
                    float spec = 0.0f;
                    if(d > 0.0f) spec = 5 * powf(d, SPECULAR_POWER);
                    projected_triangle.light_intensities[light_index] += spec;
                  }
                }
                else projected_triangle.light_intensities[light_index] = 0.0f;
              }
              projected_triangle.light_intensities[light_index] *= light_sources[light_index].intensity;
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
                projected_triangle.light_intensities[light_index] = fabsf(v_face_normal.x * lightpos_to_v_a_ray.x + v_face_normal.y * lightpos_to_v_a_ray.y + v_face_normal.z * lightpos_to_v_a_ray.z) / sqrtf(lightpos_to_v_a_ray_length_squarred);
              }
              else{
                vector3_t cam_ray = {.x = 0 - v_a.x, .y = 0 - v_a.y, .z = 0 - v_a.z}; 
                float light_v_a_dot_normal  = v_face_normal.x * lightpos_to_v_a_ray.x + v_face_normal.y * lightpos_to_v_a_ray.y + v_face_normal.z * lightpos_to_v_a_ray.z;
                float camera_v_a_dot_normal = v_face_normal.x * cam_ray.x + v_face_normal.y * cam_ray.y + v_face_normal.z * cam_ray.z;
                float spec = 0.0f;
                if(object->specular){
                  vector3_normalize_fast_approx(&lightpos_to_v_a_ray);
                  vector3_normalize_fast_approx(&cam_ray);
                  vector3_t H = vector3_add(lightpos_to_v_a_ray, cam_ray);
                  vector3_normalize_fast_approx(&H);
                  float d = fmax(vector3_dot(v_face_normal, H), 0.0f);
                  if(d > 0.0f) spec = 5 * powf(d, SPECULAR_POWER);
                }
                if((light_v_a_dot_normal > 0.0f && camera_v_a_dot_normal > 0.0f) || (light_v_a_dot_normal < 0.0f && camera_v_a_dot_normal < 0.0f))
                  projected_triangle.light_intensities[light_index] = spec + fabsf(light_v_a_dot_normal / sqrtf(lightpos_to_v_a_ray_length_squarred));
                else
                  projected_triangle.light_intensities[light_index] = 0.0f;
              } 
              projected_triangle.light_intensities[light_index] /= 1.0f + lightpos_to_v_a_ray_length_squarred;
              projected_triangle.light_intensities[light_index] *= light_sources[light_index].intensity;
            }
            else if(light_sources[light_index].type == LIGHT_AMBIENT){
              projected_triangle.light_intensities[light_index] = light_sources[light_index].intensity;
            }
            /* clamp to [0.0, 1.0] */
            if(projected_triangle.light_intensities[light_index] < 0.0f) projected_triangle.light_intensities[light_index] = 0.0f;
            else if(projected_triangle.light_intensities[light_index] > 1.0f) projected_triangle.light_intensities[light_index] = 1.0f;
          }
        }

        /* TRIANGLE RASTERIZATION */
        rasterize_triangle(
          &projected_triangle, 
          object->mesh->mesh_texture, 
          object->mesh->texture_width, object->mesh->texture_height, 
          object->blend_mode, object->alpha_cutoff,
          object->solid_color,
          object->opacity,
          object->reflective,
          object->emissive_color,
          object->emissive_intensity
        );

        total_triangles_rendered++;
      } /* for(int t = 0; t < polygon.num_vertices - 2; t++) */
    } /* for(uint32_t i = 0; i < num_mesh_faces; i++) */

    /* if object just rendered was a skybox, then paint sun over it */
    if(object->object_type == OBJECT_TYPE_SKYBOX){
      /* skybox MUST be the first object/mesh rendered.  after that we reset all zbuffer values to 0.0 so anything can be rendered on top of the skybox */
      memset(zbuffer, 0x00, total_buffer_pixels * sizeof(uint32_t)); /* zbuffer is made of floats and 0x00000000 = 0.0 */
      render_sun();
    }

    if(projection_options.object_render_debug){
      object_render_debug();
    }

  } /* for(uint32_t object_i = 0; object_i < num_objects_sorted; object_i++) */
  
  if((time_count & 0x0F) == 0)
    snprintf(text, 128, "%u, %d, %d:%d, (%.2f %.2f %.2f), (%.2f %.2f %.2f), (%.2frd %.2frd)", 
      total_triangles_rendered, fps, 
      num_objects, num_objects_sorted,
      camera.position.x, camera.position.y, camera.position.z,
      camera.direction.x, camera.direction.y, camera.direction.z,
      camera.theta, camera.eta
    );
}

void free_resources(void){
  /* free font */
  if (font) {
    TTF_CloseFont(font);
    font = NULL;
  }
  TTF_Quit();

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

  if (elapsed < FRAME_TARGET_TIME) SDL_Delay(FRAME_TARGET_TIME - elapsed);

  int64_t end_time = SDL_GetTicks64();
  int64_t frame_time = end_time - previous_frame_time;

  if (frame_time <= 0) frame_time = 1;

  fps = (int)(1000.0 / (double)frame_time);
  if((time_count & 0x0F) == 0) snprintf(fps_str, sizeof(fps_str), "fps: %d", fps);

  delta_time = frame_time / 1000.0f;
  previous_frame_time = end_time;
}

void render_text(){
  if(*text  != '\0')  draw_text(renderer, 0, 0, text);
  if(*text2 != '\0') draw_text(renderer, 0, 20, text2);
  if(*text3 != '\0') draw_text(renderer, 0, 40, text3);
}

int main(void) {

  quit = !initialize_window();
  setup();
  
  while(!quit){
    process_input();
    process_frame_rate();

    if(!is_paused){
      time_of_day += delta_time;
      snprintf(text3, 16, "%d:%d", (int)time_of_day / 60, (int)time_of_day % 60);
      update_state();
      update_and_render_color_buffer();
      time_count++; /* one count per frame */
    }

    sdl_render();
    render_text();
    SDL_RenderPresent(renderer);

  }

  destroy_window();
  free_resources();

  return 0;
}
