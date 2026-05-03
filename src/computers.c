#include <stdint.h>
#include "computers.h"

computer_t computers[MAX_NUM_COMPUTERS];
uint32_t num_computers;

void computer_push(uint32_t object_id, texture_t *screen_texture){
  computers[num_computers].object_id = object_id;
  computers[num_computers].screen_texture = screen_texture;

  num_computers++;
}