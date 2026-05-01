#ifndef ENTITY_H
#define ENTITY_H

#include <stdint.h>
#include <stdbool.h>
#include "texture.h"

#define MAX_NUM_ENTITIES 100
#define MAX_NUM_ENTITY_CHILD_OBJECTS 8
#define MAX_LEN_COMPUTER_NAME 64

typedef enum{
  ENTITY_TYPE_DOOR,
  ENTITY_TYPE_COMPUTER,
  ENTITY_TYPE_ROBOT,
  ENTITY_TYPE_BUTTON,
  ENTITY_TYPE_ITEM
} entity_type_t;

typedef struct{
  char name[MAX_LEN_COMPUTER_NAME];
  bool turned_on;
  texture_t *screen_texture;
  uint32_t cycles_per_frame;
} computer_entity_data_t;

typedef struct{
  bool open;
} door_entity_data_t;

typedef struct{
  bool pressed;
} button_entity_data_t;

typedef struct{
  bool taken;
} item_entity_data_t;

typedef struct{
  entity_type_t entity_type;
  uint32_t child_object_ids[MAX_NUM_ENTITY_CHILD_OBJECTS];
  uint32_t num_child_objects;
  union{
    computer_entity_data_t computer;
    door_entity_data_t door;
    button_entity_data_t button;
    item_entity_data_t item;
  } entity_data;
} entity_t;

extern entity_t entities[MAX_NUM_ENTITIES];
extern uint32_t num_entities;

entity_t *create_entity_computer(
  char *computer_name,
  texture_t *screen_texture,
  uint32_t cycles_per_frame
);

#endif