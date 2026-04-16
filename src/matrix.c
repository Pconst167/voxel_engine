#include <math.h>
#include "matrix.h"

vector3_t matrix3_mul_vector3(const matrix3_t *m, const vector3_t v){
  return (vector3_t){
    .x = m->element[0][0] * v.x + m->element[0][1] * v.y + m->element[0][2] * v.z,
    .y = m->element[1][0] * v.x + m->element[1][1] * v.y + m->element[1][2] * v.z,
    .z = m->element[2][0] * v.x + m->element[2][1] * v.y + m->element[2][2] * v.z,
  };
}

/*
  {sx,  0,  0,  0},
  {0,  sy,  0,  0},
  {0,  0,  sz,  0},
  {0,  0,   0,  1}
*/
matrix4_t matrix4_gen_scale(float sx, float sy, float sz){

  matrix4_t scale_matrix = {{
    {sx,   0,   0,  0},
    { 0,  sy,   0,  0},
    { 0,   0,  sz,  0},
    { 0,   0,   0,  1}
  }};

  return scale_matrix;
}

/*
  {1,  0,  0,  tx},
  {0,  1,  0,  ty},
  {0,  0,  1,  tz},
  {0,  0,  0,   1}
*/
matrix4_t matrix4_gen_trans(float tx, float ty, float tz){

  matrix4_t translation_matrix = {{
    {1,  0,  0,  tx},
    {0,  1,  0,  ty},
    {0,  0,  1,  tz},
    {0,  0,  0,   1}
  }};

  return translation_matrix;
}

/* 
  rotation order:
    rotate around x
    rotate around y
    rotate around z
  
  RzRyRx * v
*/
matrix4_t matrix4_gen_rot(float rx, float ry, float rz){
  float cx = cosf(rx);
  float sx = sinf(rx);
  float cy = cosf(ry);
  float sy = sinf(ry);
  float cz = cosf(rz); 
  float sz = sinf(rz);

  matrix4_t m = {{
    {cy*cz,  cz*sy*sx - sz*cx,  cz*sy*cx + sz*sx,  0},
    {cy*sz,  sz*sy*sx + cz*cx,  sz*sy*cx - cz*sx,  0},
    {  -sy,             cy*sx,             cy*cx,  0},
    {     0,                0,                 0,  1}
  }};

  return m;
}

matrix4_t matrix4_gen_rot_x(float rx){
  float cx = cosf(rx);
  float sx = sinf(rx);

  matrix4_t m = {{
    {1,  0,   0, 0},
    {0, cx, -sx, 0},
    {0, sx,  cx, 0},
    {0,  0,   0, 1}
  }};

  return m;
}

matrix4_t matrix4_gen_rot_y(float ry){
  float cy = cosf(ry);
  float sy = sinf(ry);

  matrix4_t m = {{
    { cy,  0,  sy,  0},
    {  0,  1,   0,  0},
    {-sy,  0,  cy,  0},
    {  0,  0,   0,  1}
  }};

  return m;
}

matrix4_t matrix4_gen_rot_z(float rz){
  float cz = cosf(rz);
  float sz = sinf(rz);

  matrix4_t m = {{
    {cz, -sz,  0,  0},
    {sz,  cz,  0,  0},
    { 0,   0,  1,  0},
    { 0,   0,  0,  1}
  }};

  return m;
}

vector4_t matrix4_mul_vector3(const matrix4_t *m, const vector3_t v){
  return (vector4_t){
    .x = m->element[0][0] * v.x + m->element[0][1] * v.y + m->element[0][2] * v.z + m->element[0][3],
    .y = m->element[1][0] * v.x + m->element[1][1] * v.y + m->element[1][2] * v.z + m->element[1][3],
    .z = m->element[2][0] * v.x + m->element[2][1] * v.y + m->element[2][2] * v.z + m->element[2][3],
    .w = 1.0f
  };
}

vector4_t matrix4_mul_vector4(const matrix4_t *m, const vector4_t v){
  return (vector4_t){
    .x = m->element[0][0] * v.x + m->element[0][1] * v.y + m->element[0][2] * v.z + m->element[0][3] * v.w,
    .y = m->element[1][0] * v.x + m->element[1][1] * v.y + m->element[1][2] * v.z + m->element[1][3] * v.w,
    .z = m->element[2][0] * v.x + m->element[2][1] * v.y + m->element[2][2] * v.z + m->element[2][3] * v.w,
    .w = m->element[3][0] * v.x + m->element[3][1] * v.y + m->element[3][2] * v.z + m->element[3][3] * v.w
  };
}

matrix4_t matrix4_mul_mat4(const matrix4_t *m1, const matrix4_t *m2){
  matrix4_t result;

  for(int row = 0; row < 4; row++){
    float a0 = m1->element[row][0];
    float a1 = m1->element[row][1];
    float a2 = m1->element[row][2];
    float a3 = m1->element[row][3];

    for(int col = 0; col < 4; col++){
      result.element[row][col] =
        a0 * m2->element[0][col] +
        a1 * m2->element[1][col] +
        a2 * m2->element[2][col] +
        a3 * m2->element[3][col];
    }
  }

  return result;
}

matrix4_t matrix4_gen_orthographic(float aspect_ratio_y){
  float scale = 1.0f; // controls zoom

  matrix4_t m = {{
    {scale*aspect_ratio_y, 0,      0,  0},
    {0,                    scale,  0,  0},
    {0,                    0,      1,  0},
    {0,                    0,      0,  1}
  }};

  return m;
}

/*
  (1/tan(fov/2)/aspect_y    0                0                    0                       )
  (0                        1/tan(fov_y/2)   0                    0                       )
  (0                        0                zfar/(zfar-znear)    -zfar*znear/(zfar-znear))
  (0                        0                1                    0                       )

  note: for x/y, basically any vertices inside the frustrum are mapped to -1 to 1 because 
  z gets mapped to 0 to 1, and x = z*tanf(+-fov/2) and so x/tanf(+-fov/2) = z which is in 0 to 1.
  same goes for y.  so the reason x/y get mapped to -1 to 1 is because they are divided by tanf(fov/2),
  and the maximum values of x/y are for when z = 1 (at zfar), and so if the bounds on x/y are -1 to 1, 
  then smaller x's are between those bounds.
*/
matrix4_t matrix4_gen_perspective(float fov_y, float aspect_ratio_y, float znear, float zfar){
  float one_over_tan_fov_y = 1.0 / tanf(fov_y / 2.0);
  
  return (matrix4_t){{
    {one_over_tan_fov_y*aspect_ratio_y,                   0,                  0,                         0},
    {                                0,  one_over_tan_fov_y,                  0,                         0},
    {                                0,                   0,  zfar/(zfar-znear),  -zfar*znear/(zfar-znear)},
    {                                0,                   0,                  1,                         0}
  }};
}

/*
  multiplying x and y by 1/tan(fov_y/2) means that if x = z*tan(fov_y/2), then x/(z*tan(fov_y/2)) = 1, and thus it means that inside the frustrum x is between -1 and 1 (same for y)
*/
vector4_t matrix4_mul_vector4_project(const matrix4_t *projection_matrix, const vector4_t v){
  vector4_t result = matrix4_mul_vector4(projection_matrix, v);

  /* w stores old z, so divide by w to create perspective divide */
  if(result.w != 0.0){
    result.x /= result.w;
    result.y /= result.w;
    result.z /= result.w;
  }

  return result;
}

/* 

  form a new set of basis vectors for the camera
    yu
    /
   /
  /_ _ _ _ _ _ zf
  \
   \
    \
     xr

  view matrix is a composite of a rotation and a translation of the world
  rotation matrix:
                   inverse                      Transpose
  (xr.x  yu.x  zf.x  0)         (xr.x  yu.x  zf.x  0)         (xr.x  xr.y  xr.z  0)
  (xr.y  yu.y  zf.y  0)    =    (xr.y  yu.y  zf.y  0)    =    (yu.x  yu.y  yu.z  0)
  (xr.z  yu.z  zf.z  0)         (xr.z  yu.z  zf.z  0)         (zf.x  zf.y  zf.z  0)
  (   0     0     0  1)         (   0     0     0  1)         (   0     0     0  1)

  inverse is equal to transpose because matrix is orthogonal (each column has length 1 and columns are orthogonal)

  translation matrix:
  (1  0  0  -cam_pos.x)
  (0  1  0  -cam_pos.y)
  (0  0  1  -cam_pos.z)
  (0  0  0           1)

  view matrix = M_rotation * M_translation

  a rotation of the original basis however, is equivalent to a change of basis, since any new basis will be a rotation of the original 
  hence our rotation matrix can be defined by a set of new basis vectors
  new basis vectors can be expressed in terms of the old ones, and then, to express all vertices in terms of the new basis
  vectors, the dot product is taken between each of the new basis vectors with each of the model's vertex
  (this means that for an arbitrary vertex/vector, we find how much of that vector is "shadowed" onto each of the new basis vectors).
  remember that the new basis vectors need to be normalized to unity.

  zf = z "forward" axis
  xr = x "right" axis
  yu = y "up" axis
*/
matrix4_t matrix4_gen_view_matrix(const vector3_t camera_position, const vector3_t camera_direction, const vector3_t up){
  vector3_t zf = camera_direction;
  /* up cross zf */
  vector3_t xr = {
    .x = up.y * zf.z - up.z * zf.y,
    .y = up.z * zf.x - up.x * zf.z,
    .z = up.x * zf.y - up.y * zf.x
  };
  vector3_normalize(&xr);
  /* zf cross xr */
  vector3_t yu = {
    .x = zf.y * xr.z - zf.z * xr.y,
    .y = zf.z * xr.x - zf.x * xr.z,
    .z = zf.x * xr.y - zf.y * xr.x
  };

  return (matrix4_t){{
    {xr.x, xr.y, xr.z, -(xr.x * camera_position.x + xr.y * camera_position.y + xr.z * camera_position.z)},
    {yu.x, yu.y, yu.z, -(yu.x * camera_position.x + yu.y * camera_position.y + yu.z * camera_position.z)},
    {zf.x, zf.y, zf.z, -(zf.x * camera_position.x + zf.y * camera_position.y + zf.z * camera_position.z)},
    {   0,    0,    0,                                                                                 1}
  }};
}

/* 
  this function combines all matrices together
  so that instead of computing each of the scale, rotation, etc matrices and then multiplying them together
  we generate the final world matrix at once, as below, and finally just multiply by the view matrix.
  the view matrix could also be combined into the world matrix and we'd save another multiplication
*/
matrix4_t matrix4_gen_world_matrix(const object_t *object){
  float cosx = cosf(object->rotation.x);
  float sinx = sinf(object->rotation.x);
  float cosy = cosf(object->rotation.y);
  float siny = sinf(object->rotation.y);
  float cosz = cosf(object->rotation.z);
  float sinz = sinf(object->rotation.z);

  float sx = object->scale.x;
  float sy = object->scale.y;
  float sz = object->scale.z;

  float tx = object->translation.x;
  float ty = object->translation.y;
  float tz = object->translation.z;

  /* this matrix is the total product of: translation*rotation*scaling */
  return (matrix4_t){{
    {sx*cosy*cosz,  sy*(cosz*siny*sinx - sinz*cosx),  sz*(cosz*siny*cosx + sinz*sinx),  tx},
    {sx*cosy*sinz,  sy*(sinz*siny*sinx + cosz*cosx),  sz*(sinz*siny*cosx - cosz*sinx),  ty},
    {    -sx*siny,                     sy*cosy*sinx,                     sz*cosy*cosx,  tz},
    {           0,                                0,                                0,   1}
  }};
}

matrix4_t matrix4_gen_world_view_matrix(const object_t *object, const matrix4_t *view_matrix){
  /* -------- WORLD -------- */
  float cosx = cosf(object->rotation.x);
  float sinx = sinf(object->rotation.x);
  float cosy = cosf(object->rotation.y);
  float siny = sinf(object->rotation.y);
  float cosz = cosf(object->rotation.z);
  float sinz = sinf(object->rotation.z);

  float sx = object->scale.x;
  float sy = object->scale.y;
  float sz = object->scale.z;

  float tx = object->translation.x;
  float ty = object->translation.y;
  float tz = object->translation.z;

  matrix4_t world_matrix = {{
    {sx*cosy*cosz,  sy*(cosz*siny*sinx - sinz*cosx),  sz*(cosz*siny*cosx + sinz*sinx),  tx},
    {sx*cosy*sinz,  sy*(sinz*siny*sinx + cosz*cosx),  sz*(sinz*siny*cosx - cosz*sinx),  ty},
    {    -sx*siny,                     sy*cosy*sinx,                     sz*cosy*cosx,  tz},
    {           0,                                0,                                0,   1}
  }};

  /* -------- MULTIPLY: view * world -------- */
  matrix4_t result;

  for(int row = 0; row < 4; row++){
      float a0 = view_matrix->element[row][0];
      float a1 = view_matrix->element[row][1];
      float a2 = view_matrix->element[row][2];
      float a3 = view_matrix->element[row][3];

      for(int col = 0; col < 4; col++){
        result.element[row][col] = a0 * world_matrix.element[0][col] +
                                   a1 * world_matrix.element[1][col] +
                                   a2 * world_matrix.element[2][col] +
                                   a3 * world_matrix.element[3][col];
      }
    }

  return result;
}