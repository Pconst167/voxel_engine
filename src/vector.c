#include "vector.h"

vector2_t vector2_neg(vector2_t v){
  vector2_t result = {
    .x = -v.x,
    .y = -v.y
  };
  return result;
}

float vector2_length(vector2_t v){
  return sqrtf(v.x * v.x + v.y * v.y);
}

vector2_t vector2_add(vector2_t a, vector2_t b){
  vector2_t result = {
    .x = a.x + b.x,
    .y = a.y + b.y
  };
  return result;
}

vector2_t vector2_sub(vector2_t a, vector2_t b){
  vector2_t result = {
    .x = a.x - b.x,
    .y = a.y - b.y
  };
  return result;
}

vector2_t vector2_mul(vector2_t v, float factor){
  vector2_t result = {
    .x = v.x * factor,
    .y = v.y * factor
  };
  return result;
}

vector2_t vector2_div(vector2_t v, float factor){
  vector2_t result = {
    .x = v.x / factor,
    .y = v.y / factor
  };
  return result;
}

float vector2_dot(vector2_t a, vector2_t b){
  return a.x * b.x + a.y * b.y;
}

void vector2_normalize(vector2_t *v){
  float len = 1.0f / sqrtf(v->x * v->x + v->y * v->y);

  v->x *= len;
  v->y *= len;
}

/* vector3 functions */
float vector3_length(vector3_t v){
  return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

float vector3_length_squarred(vector3_t v){
  return v.x * v.x + v.y * v.y + v.z * v.z;
}

vector3_t vector3_neg(vector3_t v){
  vector3_t result = {
    .x = -v.x,
    .y = -v.y,
    .z = -v.z
  };
  return result;
}

vector3_t vector3_add(vector3_t a, vector3_t b){
  vector3_t result = {
    .x = a.x + b.x,
    .y = a.y + b.y,
    .z = a.z + b.z
  };
  return result;
}

vector3_t vector3_sub(vector3_t a, vector3_t b){
  vector3_t result = {
    .x = a.x - b.x,
    .y = a.y - b.y,
    .z = a.z - b.z
  };
  return result;
}

vector3_t vector3_mul(vector3_t v, float factor){
  vector3_t result = {
    .x = v.x * factor,
    .y = v.y * factor,
    .z = v.z * factor
  };
  return result;
}

vector3_t vector3_div(vector3_t v, float factor){
  vector3_t result = {
    .x = v.x / factor,
    .y = v.y / factor,
    .z = v.z / factor
  };
  return result;
}

vector3_t vector3_cross(vector3_t a, vector3_t b){
  vector3_t result = {
    .x = a.y * b.z - a.z * b.y,
    .y = a.z * b.x - a.x * b.z,
    .z = a.x * b.y - a.y * b.x
  };
  return result;
}

float vector3_dot(vector3_t a, vector3_t b){
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

vector3_t vector3_rotate_x(vector3_t v, float angle){
  float s = sinf(angle);
  float c = cosf(angle);

  vector3_t rotated_vector = {
    .x = v.x,
    .y = v.y * c - v.z * s,
    .z = v.y * s + v.z * c
  };
  return rotated_vector;
}

vector3_t vector3_rotate_y(vector3_t v, float angle){
  float s = sinf(angle);
  float c = cosf(angle);

  vector3_t rotated_vector = {
    .x = v.x * c - v.z * s,
    .y = v.y,
    .z = v.x * s + v.z * c
  };
  return rotated_vector;
}

vector3_t vector3_rotate_z(vector3_t v, float angle){
  float s = sinf(angle);
  float c = cosf(angle);

  vector3_t rotated_vector = {
    .x = v.x * c - v.y * s,
    .y = v.x * s + v.y * c,
    .z = v.z
  };
  return rotated_vector;
}

inline void vector3_normalize(vector3_t *v){
  float len2 = v->x * v->x + v->y * v->y + v->z * v->z;

  if (len2 > 0.0f) {
    float inv_len = 1.0f / sqrtf(len2);
    v->x *= inv_len;
    v->y *= inv_len;
    v->z *= inv_len;
  }
}

inline void vector3_normalize_fast_approx(vector3_t *v){
  float len2 = v->x * v->x + v->y * v->y + v->z * v->z;

  if (len2 > 0.0f) {
    float inv_len = q_rsqrt(len2);
    v->x *= inv_len;
    v->y *= inv_len;
    v->z *= inv_len;
  }
}

vector4_t vector4_from_vector3(vector3_t v){
  vector4_t result = {
    v.x,
    v.y,
    v.z,
    1.0
  };

  return result;
}

vector3_t vector3_from_vector4(vector4_t v){
  vector3_t result = {
    v.x,
    v.y,
    v.z
  };

  return result;
}
