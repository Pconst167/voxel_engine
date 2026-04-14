#ifndef ASSET_H
#define ASSET_H

#include "mesh.h"

#define MAX_ASSETS 1000
#define MAX_ASSET_FILENAME_LEN 256

/*
  check if asset with same .obj filename exists
  if exists return mesh pointer
  if not, then create new asset for that filename and load mesh object/texture and return pointer to mesh

*/
typedef struct{
  char obj_filename[MAX_ASSET_FILENAME_LEN];
  char texture_filename[MAX_ASSET_FILENAME_LEN];
  mesh_t mesh;
} asset_t;

extern asset_t assets[MAX_ASSETS];
extern int num_assets;

#endif