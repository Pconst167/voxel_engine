#include "collision.h"
#include "debug.h"

collision_aabb_t collision_aabbs[MAX_NUM_COLLISION_AABBS];
uint32_t num_collision_aabbs;
uint32_t collision_step = 1; /* starts at 1 because each collision aabb's initial step is reset to 0 - ensures aabbs don't miss the first collision step */

uint32_t create_collision_aabb(
  aabb_3d_t aabb,
  uint32_t owner_object_id,
  bool active
){
  collision_aabbs[num_collision_aabbs].aabb            = aabb;
  collision_aabbs[num_collision_aabbs].owner_object_id = owner_object_id;
  collision_aabbs[num_collision_aabbs].active          = active;
  collision_aabbs[num_collision_aabbs].collision_step  = 0;

  num_collision_aabbs++;

  return num_collision_aabbs - 1;
}