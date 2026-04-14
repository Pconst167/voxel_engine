#ifndef VECTOR_H
#define VECTOR_H

typedef struct{
  float x, y;
} vector2_t;

typedef struct{
  float x, y, z;
} vector3_t;

typedef struct{
  float x, y, z, w;
} vector4_t;

/* vector2 functions */
vector2_t vector2_neg(vector2_t v);
float vector2_length(vector2_t v);
void vector2_normalize(vector2_t *v);
vector2_t vector2_add(vector2_t a, vector2_t b);
vector2_t vector2_sub(vector2_t a, vector2_t b);
vector2_t vector2_mul(vector2_t v, float factor);
vector2_t vector2_div(vector2_t v, float factor);

/* vector3 functions */
vector3_t vector3_neg(vector3_t v);
float vector3_length(vector3_t v);
float vector3_length_squarred(vector3_t v);
void vector3_normalize(vector3_t *v);
void vector3_normalize_fast_approx(vector3_t *v);

vector3_t vector3_add(vector3_t a, vector3_t b);
vector3_t vector3_sub(vector3_t a, vector3_t b);
vector3_t vector3_mul(vector3_t v, float factor);
vector3_t vector3_div(vector3_t v, float factor);

float vector3_dot(vector3_t a, vector3_t b);
vector3_t vector3_cross(vector3_t a, vector3_t b);

vector3_t vector3_rotate_x(vector3_t v, float angle);
vector3_t vector3_rotate_y(vector3_t v, float angle);
vector3_t vector3_rotate_z(vector3_t v, float angle);

vector3_t vector3_from_vector4(vector4_t v);
vector4_t vector4_from_vector3(vector3_t v);

#endif