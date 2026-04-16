#ifndef LIGHT_H
#define LIGHT_H

#include "vector.h"
#include "color.h"
#include "misc.h"

#define MAX_NUM_LIGHT_SOURCES 100

typedef enum{
  LIGHT_DIRECTIONAL,
  LIGHT_POINT,
  LIGHT_AMBIENT
} light_type_t;

typedef struct{
  float red;
  float green;
  float blue;
} light_intensity_rgb_t;

typedef struct{
  light_type_t type;
  vector3_t position;
  vector3_t direction;
  color_t color;
  float intensity;
} light_t;

extern light_t light_sources[MAX_NUM_LIGHT_SOURCES];
extern uint32_t num_light_sources;

void initialize_lights(void);
uint32_t create_light_source(light_type_t type, vector3_t position, vector3_t direction, color_t color, float intensity);
light_intensity_rgb_t find_total_light_intensity(const float *light_intensities);

#endif