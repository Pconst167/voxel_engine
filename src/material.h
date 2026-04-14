#ifndef MATERIAL_H
#define MATERIAL_H

typedef struct{
  union{
    uint32_t uint32;
    struct{
      uint8_t red;
      uint8_t green;
      uint8_t blue;
      uint8_t alpha;
    } rgb;
  };
} color_t;



#endif