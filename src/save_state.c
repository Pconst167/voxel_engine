#include <stdio.h>
#include <stdlib.h>
#include "save_state.h"
#include "player.h"
#include "chunk.h"
#include "block.h"
#include "log.h"

bool save_chunk(int32_t chunk_y, int32_t chunk_z, int32_t chunk_x){
  char filename[256];

  sprintf(filename, "./save/chunk_%d_%d_%d.dat", chunk_y, chunk_z, chunk_x);

  FILE *fp = fopen(filename, "wb+");
  if(fp == NULL){
    print_log("[ERROR] unable to create save file");
    return false;
  }

  chunk_t *chunk = &chunks[chunk_y][chunk_z][chunk_x];

  save_state_t *save_state = malloc(sizeof(save_state_t));
  if(save_state == NULL){
    fclose(fp);
    return false;
  }

  // blocks
  int32_t save_state_block_y = 0; // the save state array indices themselves go from 0 to CHUNK_SIZE - 1
  int32_t save_state_block_z = 0;
  int32_t save_state_block_x = 0;
  for(uint32_t y = chunk->y_plane_start; y < chunk->y_plane_start + CHUNK_SIZE; y++){
    save_state_block_z = 0;

    for(uint32_t z = chunk->z_row_start; z < chunk->z_row_start + CHUNK_SIZE; z++){
      save_state_block_x = 0;

      for(uint32_t x = chunk->x_col_start; x < chunk->x_col_start + CHUNK_SIZE; x++){
        save_state->blocks[save_state_block_y][save_state_block_z][save_state_block_x] = block_at(y, z, x);
        save_state_block_x++;
      }

      save_state_block_z++;
    }

    save_state_block_y++;
  }

  fwrite(save_state, sizeof(save_state_t), 1, fp);

  fclose(fp);

  return true;
}

bool load_chunk(int32_t chunk_y, int32_t chunk_z, int32_t chunk_x){
  char filename[256];

  snprintf(filename, sizeof(filename), "./save/chunk_%d_%d_%d.dat", chunk_y, chunk_z, chunk_x);

  FILE *fp = fopen(filename, "rb");
  if(fp == NULL){
    return false;
  }

  save_state_t *save_state = malloc(sizeof(save_state_t));
  if(save_state == NULL){
    fclose(fp);
    return false;
  }

  size_t items_read = fread(save_state, sizeof(save_state_t), 1, fp);
  fclose(fp);

  if(items_read != 1){
    print_log("[ERROR] failed to read chunk save file");
    return false;
  }

  chunk_t *chunk = &chunks[chunk_y][chunk_z][chunk_x];

  int32_t save_state_block_y = 0;
  int32_t save_state_block_z = 0;
  int32_t save_state_block_x = 0;

  for(uint32_t y = chunk->y_plane_start; y < chunk->y_plane_start + CHUNK_SIZE; y++){
    save_state_block_z = 0;

    for(uint32_t z = chunk->z_row_start; z < chunk->z_row_start + CHUNK_SIZE; z++){
      save_state_block_x = 0;

      for(uint32_t x = chunk->x_col_start; x < chunk->x_col_start + CHUNK_SIZE; x++){
        block_at(y, z, x) = save_state->blocks[save_state_block_y][save_state_block_z][save_state_block_x];
        save_state_block_x++;
      }

      save_state_block_z++;
    }

    save_state_block_y++;
  }

  return true;
}

bool load_all_saved_chunks(void){
  bool loaded_any_chunks = false;

  for(int32_t chunk_y = 0; chunk_y < NUM_CHUNK_DIVISIONS_Y; chunk_y++){
    for(int32_t chunk_z = 0; chunk_z < NUM_CHUNK_DIVISIONS_Z; chunk_z++){
      for(int32_t chunk_x = 0; chunk_x < NUM_CHUNK_DIVISIONS_X; chunk_x++){
        if(load_chunk(chunk_y, chunk_z, chunk_x)){
          print_log("[INFO] loaded chunk: %d:%d:%d", chunk_y, chunk_z, chunk_x);
          loaded_any_chunks = true;
        }
      }
    }
  }

  return loaded_any_chunks;
}