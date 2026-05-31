#ifndef VOLCANO_H
#define VOLCANO_H

typedef struct{
  int32_t width;
  int32_t height;
} volcano_t;

void place_volcano(
  int32_t z_row,
  int32_t x_col,
  int32_t size, int32_t height
);

void place_lava_pool(int32_t z_row, int32_t x_col);

#endif