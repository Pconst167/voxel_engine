#include "color.h"

color_t color_from_float_rgb(float red, float green, float blue){
  if(red   < 0.0f) red = 0.0f;
  if(green < 0.0f) green = 0.0f;
  if(blue  < 0.0f) blue = 0.0f;

  if(red   > 1.0f) red = 1.0f;
  if(green > 1.0f) green = 1.0f;
  if(blue  > 1.0f) blue = 1.0f;

  color_t color;
  color.rgb.red   = (uint8_t)(red   * 255.0f);
  color.rgb.green = (uint8_t)(green * 255.0f);
  color.rgb.blue  = (uint8_t)(blue  * 255.0f);
  color.rgb.alpha = 255;

  return color;
}