#ifndef MATRIX_H
#define MATRIX_H

#include "vector.h"
#include "object.h"
#include "camera.h"

#define MAT4_IDENTITY {{{1.0,0.0,0.0,0.0}, {0.0,1.0,0.0,0.0}, {0.0,0.0,1.0,0.0}, {0.0,0.0,0.0,1.0}}}

typedef struct{
  float element[3][3];
} matrix3_t;

typedef struct{
  float element[4][4];
} matrix4_t;

vector3_t matrix3_mul_vector3(const matrix3_t *m, const vector3_t v);

matrix4_t matrix4_gen_scale(float sx, float sy, float sz);
matrix4_t matrix4_gen_trans(float tx, float ty, float tz);
matrix4_t matrix4_gen_rot(float rx, float ry, float rz);

matrix4_t matrix4_gen_rot_x(float rx);
matrix4_t matrix4_gen_rot_y(float ry);
matrix4_t matrix4_gen_rot_z(float rz);

vector4_t matrix4_mul_vector3(const matrix4_t *m, const vector3_t v);
vector4_t matrix4_mul_vector4(const matrix4_t *m, const vector4_t v);
matrix4_t matrix4_mul_mat4(const matrix4_t *m1, const matrix4_t *m2);

matrix4_t matrix4_gen_perspective(float fov_y, float aspect_ratio_y, float znear, float zfar);
matrix4_t matrix4_gen_orthographic(float aspect_ratio_y);


vector4_t matrix4_mul_vector4_project(const matrix4_t *projection_matrix, const vector4_t v);

matrix4_t matrix4_gen_view_matrix(const vector3_t camera_position, const vector3_t camera_direction, const vector3_t up);

matrix4_t matrix4_gen_world_matrix(const object_t *object);

matrix4_t matrix4_gen_world_view_matrix(const object_t *object, const matrix4_t *view_matrix);

#endif