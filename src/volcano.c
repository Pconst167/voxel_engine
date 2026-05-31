#include <stdint.h>
#include "volcano.h"
#include "block.h"
#include "misc.h"

void place_volcano(
  int32_t z_row,
  int32_t x_col,
  int32_t size, int32_t height
){
  // find first solid block along y
  int32_t first_solid_y = 0;
  for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
    if(block_materials[block_at(y, z_row, x_col).type].solid){
      first_solid_y = y;
      break;
    }
  }

  for(int32_t y = first_solid_y; y < first_solid_y + height; y++){
    float y_factor = -(float)y / (float)height + (float)first_solid_y/(float)height + 1.0f;
    for(int32_t z = z_row - (int32_t)(y_factor*size / 2.0f); z < z_row + (int32_t)(y_factor*size / 2.0f); z++){
      for(int32_t x = x_col - (int32_t)(y_factor*(float)size / 2.0f); x < x_col + (int32_t)(y_factor*(float)size / 2.0f); x++){
        if(z >= z_row - 1 && z <= z_row + 1 && x >= x_col - 1 && x <= x_col + 1)
          assign_block_material(y, z, x, BLOCK_TYPE_LAVA1);
        else
          if(xorshift32() % 2 == 0)
            assign_block_material(y, z, x, BLOCK_TYPE_VOLCANIC3);
          else
            assign_block_material(y, z, x, BLOCK_TYPE_VOLCANIC0);
      }
    }
  }
}

void place_lava_pool(int32_t z_row, int32_t x_col){
  // find first solid block along y
  int32_t first_solid_y = 0;
  for(int32_t y = MAP_NUM_BLOCKS_Y - 1; y >= 0; y--){
    if(block_materials[block_at(y, z_row, x_col).type].solid){
      first_solid_y = y;
      break;
    }
  }

  for(int32_t z = z_row; z <= z_row + 8; z++){
    for(int32_t x = x_col; x <= x_col + 8; x++){
      if(z == z_row || z == z_row + 8 || x == x_col || x == x_col + 8){
        if(xorshift32() < 1000)
          assign_block_material(first_solid_y, z, x, BLOCK_TYPE_LAVA1);
        else
          assign_block_material(first_solid_y, z, x, BLOCK_TYPE_VOLCANIC3);
      }
      else{
        assign_block_material(first_solid_y, z, x, BLOCK_TYPE_LAVA1);
      }
    }

  }
}