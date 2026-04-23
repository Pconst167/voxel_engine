#ifndef COLLISION_H
#define COLLISION_H

#include <stdint.h>
#include "aabb.h"

#define MAX_NUM_COLLISION_AABBS 100

typedef struct{
  aabb_3d_t aabb;
  /* each time we process a collision check (for each axis), increment the step count
     so that we do not recheck the same AABB twice since AABBs can repeat across chunks */
  uint32_t collision_step_id; 
} collision_aabb_t;

extern collision_aabb_t collision_aabbs[MAX_NUM_COLLISION_AABBS];
extern uint32_t num_collision_aabbs;


#endif