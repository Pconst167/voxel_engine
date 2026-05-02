#ifndef COMPUTERS_H
#define COMPUTERS_H

#define MAX_NUM_COMPUTERS 10

typedef struct{
  uint32_t object_id;
} computer_t;

extern computer_t computers[MAX_NUM_COMPUTERS];
extern uint32_t num_computers;

void computer_push(uint32_t object_id);

#endif