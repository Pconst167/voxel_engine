#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "entity.h"
#include "log.h"

entity_t entities[MAX_NUM_ENTITIES];
uint32_t num_entities;

entity_t *create_entity_computer(
  char *computer_name,
  texture_t *screen_texture,
  uint32_t cycles_per_frame
){
  if(num_entities == MAX_NUM_ENTITIES){
    print_log("ERROR: maximum number of entities reached: %u", MAX_NUM_ENTITIES);
    return NULL;
  }

  entity_t *entity = &entities[num_entities];

  strcpy(entity->entity_data.computer.name, computer_name);
  entity->entity_type                           = ENTITY_TYPE_COMPUTER;
  entity->num_child_objects                     = 0;
  entity->entity_data.computer.cycles_per_frame = cycles_per_frame;
  entity->entity_data.computer.screen_texture   = screen_texture;
  entity->entity_data.computer.turned_on        = false;

  num_entities++;

  return entity;
}