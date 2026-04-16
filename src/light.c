#include <stdint.h>
#include "light.h"

/*
  NOTE ABOUT POINT LIGHTS:
  for a cube, even on the same face, the intensity values for a point light can be slightly different
  because the ray from the point light to each face has a different lenght!
  so even though the cube face is the same, each of thw two faces has a different position.
  this could be solved in a few ways, but for now it behaves like that.
*/

light_t light_sources[MAX_NUM_LIGHT_SOURCES];
uint32_t num_light_sources = 0;

/* normalize all light direction vectors */
void initialize_lights(){
  for(uint32_t light_index = 0; light_index < num_light_sources; light_index++){
    vector3_normalize(&light_sources[light_index].direction);
  }
}

uint32_t create_light_source(light_type_t type, vector3_t position, vector3_t direction, color_t color, float intensity){
  light_sources[num_light_sources].type = type;
  light_sources[num_light_sources].position.x = position.x;
  light_sources[num_light_sources].position.y = position.y;
  light_sources[num_light_sources].position.z = position.z;
  light_sources[num_light_sources].direction.x = direction.x;
  light_sources[num_light_sources].direction.y = direction.y;
  light_sources[num_light_sources].direction.z = direction.z;
  light_sources[num_light_sources].color = color;
  light_sources[num_light_sources].intensity = intensity;

  num_light_sources++;

  return num_light_sources - 1;
}

/*
  for each light source, we take each color channel: red, green, blue, and multiply each channel by the amount of red, green, blue in the pixel
  respectively. we also multiply each channel by the intensity factor which comes from the face's normal vector dotted with the light ray.
  then we add all channels together: red + red + ..., green + green + ..., blue + blue + ...
  this gives the total amount of color/intensity per channel.
  finally we must clamp the total to 100%, because we can't have an intensity larger than 100%/255
  NOTE: -1.0 to 1.0 intensity clamp is done when calculating the intensity values 
*/
__attribute__((optimize("Ofast")))
light_intensity_rgb_t find_total_light_intensity(const float *light_intensities){
  light_intensity_rgb_t intensity = {0.0f, 0.0f, 0.0f};

  for(uint32_t light_index = 0; light_index < num_light_sources; light_index++){
    color_t light_color   = light_sources[light_index].color;
    float light_intensity = light_intensities[light_index];

    /* for each light color accumulate the total amount/brightness of that color */
    intensity.red   += light_color.rgb.red   * light_intensity;
    intensity.green += light_color.rgb.green * light_intensity;
    intensity.blue  += light_color.rgb.blue  * light_intensity;
  }

  intensity.red   *= 1.0f / 255.0f;
  intensity.green *= 1.0f / 255.0f;
  intensity.blue  *= 1.0f / 255.0f;

  /* return final intensity */
  return intensity;
}