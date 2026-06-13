#ifndef BLOCK_H
#define BLOCK_H

#include "aabb.h"
#include "material.h"
#include "mesh.h"
#include "light.h"
#include "sound.h"
#include "player_tools.h"

#define MAP_NUM_BLOCKS_X 512
#define MAP_NUM_BLOCKS_Z 512
#define MAP_NUM_BLOCKS_Y 256
#define CHUNK_SIZE 16
#define NUM_CHUNK_DIVISIONS_Y (MAP_NUM_BLOCKS_Y / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_Z (MAP_NUM_BLOCKS_Z / CHUNK_SIZE)
#define NUM_CHUNK_DIVISIONS_X (MAP_NUM_BLOCKS_X / CHUNK_SIZE)
#define CHUNK_RENDER_DISTANCE_Y 2 // render this number of chunks around the player in XZ
#define CHUNK_RENDER_DISTANCE_Z 2 // render this number of chunks around the player in XZ
#define CHUNK_RENDER_DISTANCE_X 2 // render this number of chunks around the player in XZ

// SURFACE LEVEL DATA
#define SURFACE_LEVEL (0.4f * MAP_NUM_BLOCKS_Y)

#define BLOCK_FACE_SHIFT_DOWN_AMOUNT (0.1f) // for fluid blocks

#define MAX_BLOCK_LOOK_AT_DISTANCE 6.0f
#define BLOCK_TYPE_STRING_NAME_MAX_LEN 32

// macro to get block at coordiates x y z
#define MAP_X_STRIDE 1
#define MAP_Z_STRIDE MAP_NUM_BLOCKS_X
#define MAP_Y_STRIDE (MAP_NUM_BLOCKS_X * MAP_NUM_BLOCKS_Z)
#define block_at(y, z, x) blocks[(y) * MAP_Y_STRIDE + (z) * MAP_Z_STRIDE + (x)]

// BLOCK LIGHT
#define MAX_LIGHT_QUEUE_SIZE 1048576
#define MAX_SKY_LIGHT 10
#define MAX_SKY_LIGHT_FLOAT 10.0f
#define MAX_BLOCK_LIGHT 20
#define MAX_BLOCK_LIGHT_FLOAT 20.0f
#define MIN_TOTAL_LIGHT (1.0f / 21.0f)

// WORLD PARAMETERS
#define WORLD_BASE_SURFACE_LEVEL (0.5f * MAP_NUM_BLOCKS_Y)
#define WORLD_MAX_HILL_HEIGHT    20
#define WORLD_UNDER_MAX_HEIGHT    8

#define WORLD_DIRT0_DEPTH        6
#define WORLD_STONE0_DEPTH       6
#define WORLD_WATER_LEVEL        (WORLD_BASE_SURFACE_LEVEL - 10)
#define WORLD_LARGE_HILL_SCALE   0.02f
#define WORLD_SMALL_HILL_SCALE   0.03f
#define WORLD_MOUNTAIN_SCALE     0.01f
#define WORLD_LAVA_POOL_SCALE    0.10f

#define WORLD_OVERHANG_SCALE     0.04f

#define WORLD_BASE_CAVE_LEVEL    WORLD_WATER_LEVEL - 10
#define WORLD_SMALL_CAVE_SCALE   0.05f
#define WORLD_MEDIUM_CAVE_SCALE  0.04f
#define WORLD_LARGE_CAVE_SCALE   0.03f
#define WORLD_UNDER_NOISE_SCALE   0.3f

// WORLD LAYERS
#define WORLD_BASE_LAVA_LEVEL      (0.00f * MAP_NUM_BLOCKS_Y)
#define WORLD_BASE_VOLCANIC0_LEVEL (0.01f * MAP_NUM_BLOCKS_Y)
#define WORLD_BASE_VOLCANIC1_LEVEL (0.02f * MAP_NUM_BLOCKS_Y)
#define WORLD_BASE_VOLCANIC2_LEVEL (0.04f * MAP_NUM_BLOCKS_Y)
#define WORLD_BASE_STONE_LEVEL     (0.08f * MAP_NUM_BLOCKS_Y)

typedef struct{
  int32_t y;
  int32_t z;
  int32_t x;
  uint8_t light;
} light_node_t;

typedef struct{
  light_node_t nodes[MAX_LIGHT_QUEUE_SIZE];

  uint32_t head;
  uint32_t tail;
} light_queue_t;

typedef struct{
  uint8_t sky_light;
  uint8_t block_light;
} light_sample_t;

// BLOCK TYPES
typedef enum{
  BLOCK_TYPE_AIR        ,
  BLOCK_TYPE_LAVA0      ,
  BLOCK_TYPE_LAVA1      ,
  BLOCK_TYPE_VOLCANIC0  ,
  BLOCK_TYPE_VOLCANIC1  ,
  BLOCK_TYPE_VOLCANIC2  ,
  BLOCK_TYPE_BEDROCK    ,
  BLOCK_TYPE_STONE0     ,
  BLOCK_TYPE_STONE1     ,
  BLOCK_TYPE_STONE2     ,
  BLOCK_TYPE_STONE3     ,
  BLOCK_TYPE_STONE4     ,
  BLOCK_TYPE_STONE5     ,
  BLOCK_TYPE_STONE6     ,
  BLOCK_TYPE_STONE7     ,
  BLOCK_TYPE_STONE8     ,
  BLOCK_TYPE_COAL       ,
  BLOCK_TYPE_IRON       ,
  BLOCK_TYPE_DIAMOND    ,
  BLOCK_TYPE_SILVER     ,
  BLOCK_TYPE_GOLD       ,
  BLOCK_TYPE_DIRT0       ,
  BLOCK_TYPE_DIRT1       ,
  BLOCK_TYPE_GRASS      ,
  BLOCK_TYPE_CHERRY_GRASS,
  BLOCK_TYPE_SAND0,
  BLOCK_TYPE_CLAY0,
  BLOCK_TYPE_TREE_TRUNK0,
  BLOCK_TYPE_TREE_TRUNK1,
  BLOCK_TYPE_TREE_TRUNK2,
  BLOCK_TYPE_TREE_TRUNK3,
  BLOCK_TYPE_TREE_TRUNK4,
  BLOCK_TYPE_TREE_TRUNK5,
  BLOCK_TYPE_TREE_TRUNK6,
  BLOCK_TYPE_TREE_TRUNK7,
  BLOCK_TYPE_LEAVES0    ,
  BLOCK_TYPE_LEAVES1    ,
  BLOCK_TYPE_LEAVES2    ,
  BLOCK_TYPE_LEAVES3    ,
  BLOCK_TYPE_LEAVES4    ,
  BLOCK_TYPE_LEAVES5    ,
  BLOCK_TYPE_LEAVES6    ,
  BLOCK_TYPE_LEAVES7    ,
  BLOCK_TYPE_LEAVES8    ,
  BLOCK_TYPE_LEAVES9    ,
  BLOCK_TYPE_FLOWER_LEAVES0,
  BLOCK_TYPE_FLOWER_LEAVES1,
  BLOCK_TYPE_WATER0     ,
  BLOCK_TYPE_BOG0       ,
  BLOCK_TYPE_GLASS      ,
  BLOCK_TYPE_LAMP0      ,
  BLOCK_TYPE_OVEN0      ,
  BLOCK_TYPE_CHEST0     ,
} block_type_t;

typedef enum{
  BLOCK_FLUID_NONE,
  BLOCK_FLUID_WATER,
  BLOCK_FLUID_QUICKSAND,
  BLOCK_FLUID_LAVA
} block_fluid_type_t;

typedef enum{
  BLOCK_ORIENTATION_LEFT,
  BLOCK_ORIENTATION_RIGHT,
  BLOCK_ORIENTATION_FRONT,
  BLOCK_ORIENTATION_BACK
} block_orientation_t;

typedef enum{
  BLOCK_FACE_TOP,
  BLOCK_FACE_BOTTOM,
  BLOCK_FACE_LEFT,
  BLOCK_FACE_RIGHT,
  BLOCK_FACE_FRONT,
  BLOCK_FACE_BACK,
} block_face_direction_t;

typedef struct{
  char *material_name;
  // physical properties
  int32_t mass;
  bool solid;
  player_tool_class_t preferred_tool_class;
  float wrong_tool_multiplier;
  bool mineable;
  block_fluid_type_t block_fluid_type;
  bool transparent_to_light;
  // rendering
  bool backface_culling;
  // color and light
  color_t base_color;
  color_t emissive_color;   //  object's own emission color */
  bool reflects_light;
  // alpha
  blend_mode_t blend_mode;
  uint8_t alpha_cutoff;
  // texture data
  texture_t *front_texture;
  texture_t *back_texture;
  texture_t *left_texture;
  texture_t *right_texture;
  texture_t *top_texture;
  texture_t *bottom_texture;
  // sound type
  block_sound_type_t sound_type;
} block_material_t;

typedef struct{
  block_face_direction_t block_face_type;
  vector3_t points[4];
  uv_t uv[6];
  light_intensity_rgb_t total_light_intensity;
} block_face_t;

typedef struct{
  block_type_t type;
  block_orientation_t orientation;
  int32_t current_mass;
  float temperature;
  aabb_3d_t aabb;
  float avg_w;
  bool looked_at;
  uint8_t sky_light;
  uint8_t block_light;
} block_t;

typedef struct{
  int32_t hit;
  int32_t block_x;
  int32_t block_y;
  int32_t block_z;
  int32_t normal_x;
  int32_t normal_y;
  int32_t normal_z;
  float distance;
} block_raycast_hit_t;

// keep block type names as strings
extern char *block_type_names[];

extern block_material_t *block_materials;

// Y slowest changing dimension, X is fastest changing
extern block_t *blocks;

// block currently being looked at
extern block_raycast_hit_t looked_at_block;

// block light queue
extern light_queue_t light_queue;
extern const int32_t neighbour_y_offsets[6];
extern const int32_t neighbour_z_offsets[6];
extern const int32_t neighbour_x_offsets[6];

void init_block_materials();
void init_block_map();
void get_block_range_from_aabb(const aabb_3d_t *aabb, int *start_x, int *end_x, int *start_y, int *end_y, int *start_z, int *end_z);
block_face_t generate_block_face(block_face_direction_t block_face_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col);
block_face_t generate_fluid_top_block_face(block_face_direction_t block_face_type, uint32_t y_plane, uint32_t z_row, uint32_t x_col);
bool should_render_face(block_face_direction_t face_direction, int32_t y, int32_t z, int32_t x);
bool should_render_fluid_block_connectors(block_face_direction_t face_direction, int32_t y, int32_t z, int32_t x);
bool should_shift_block_faces(int32_t y_plane, int32_t z_row, int32_t x_col);

block_raycast_hit_t raycast_blocks(vector3_t ray_origin, vector3_t ray_direction, float max_distance);
void assign_block_material(int32_t block_y, int32_t block_z, int32_t block_x, block_type_t block_type);

void set_block_orientation(int32_t block_y, int32_t block_z, int32_t block_x, vector3_t player_position);

char *get_block_material_name(block_type_t block_type);

bool block_is_transparent(block_t *block);
bool block_is_opaque(block_t *block);
bool block_is_emissive(block_t *block);
bool block_is_stone(block_t *block);
bool block_is_tree_leaves(block_t *block);
bool block_is_tree_trunk(block_t *block);
bool block_is_water(block_t *block);
bool block_is_surface_fluid(int32_t y, int32_t z, int32_t x);
light_sample_t get_fluid_surface_light(int32_t y, int32_t z, int32_t x);

void place_pond(int32_t z_row, int32_t x_col);

// BLOCK LIGHT FUNCTIONS
void reset_light_around_area(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end);
void seed_initial_sky_light(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end);
void seed_initial_block_light(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end);
light_node_t light_queue_pop();
bool light_queue_not_empty();
void light_queue_push(light_node_t light_node);
void propagate_sky_light(void);
void propagate_block_light(void);
void remove_sky_light_from_area(int32_t y, int32_t z, int32_t x);
void seed_sky_light_around_area(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end);
void seed_block_light_around_area(int32_t y_start, int32_t y_end, int32_t z_start, int32_t z_end, int32_t x_start, int32_t x_end);



#endif