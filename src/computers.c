#include <stdint.h>
#include "computers.h"

computer_t computers[MAX_NUM_COMPUTERS];
uint32_t num_computers;

void computer_push(uint32_t object_id){
  computers[num_computers].object_id = object_id;

  num_computers++;
}