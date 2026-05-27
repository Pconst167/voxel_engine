#include <stdio.h>
#include "save_state.h"
#include "player.h"
#include "log.h"

bool save(){
  FILE *fp = fopen("./save/save", "wb+");
  if(fp == NULL){
    print_log("[ERROR] unable to create save file");
    return false;
  }

  // player data
  fwrite(&player.position, sizeof(vector3_t), 1, fp);
  fwrite(&player.theta, sizeof(float), 1, fp);
  fwrite(&player.eta, sizeof(float), 1, fp);

  // blocks
  fwrite(blocks, sizeof(block_t), MAP_NUM_BLOCKS_Y * MAP_NUM_BLOCKS_Z * MAP_NUM_BLOCKS_X, fp);

  fclose(fp);

  return true;
}

bool load(){
  FILE *fp = fopen("./save/save", "rb");
  if(fp == NULL){
    print_log("[ERROR] unable to load save file");
    return false;
  }

  // player data
  size_t items_read;
  items_read = fread(&player.position, sizeof(vector3_t), 1, fp);
  if(items_read < 1) print_log("[ERROR] failed to read player position from save file");
  items_read = fread(&player.theta, sizeof(float), 1, fp);
  if(items_read < 1) print_log("[ERROR] failed to read player 'theta' from save file");
  items_read = fread(&player.eta, sizeof(float), 1, fp);
  if(items_read < 1) print_log("[ERROR] failed to read player 'eta' from save file");

  // blocks
  items_read = fread(blocks, sizeof(block_t), MAP_NUM_BLOCKS_Y * MAP_NUM_BLOCKS_Z * MAP_NUM_BLOCKS_X, fp);
  if(items_read < MAP_NUM_BLOCKS_Y * MAP_NUM_BLOCKS_Z * MAP_NUM_BLOCKS_X) print_log("[ERROR] failed to read all blocks from save file");

  fclose(fp);

  return true;
}