#ifndef COMPUTER_H
#define COMPUTER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct{
  bool turned_on;
  uint32_t cycles_per_frame;

} computer_t;


#endif