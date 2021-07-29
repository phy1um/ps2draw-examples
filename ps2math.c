
#include "ps2math.h"
#include <math.h>

// matrices
void p2m_m4f_make_identity(m4f mat) {
  for (int i = 0; i < 16; i++) {
    mat[i] = i % 5 == 0 ? 1 : 0;
  }
}

void p2m_mul_m4f_v4f(m4f mat, v4f vec, v4f out) {
  out[0] =
      vec[0] * mat[0] + vec[1] * mat[1] + vec[2] * mat[2] + vec[3] * mat[3];
  out[1] =
      vec[0] * mat[4] + vec[1] * mat[5] + vec[2] * mat[6] + vec[3] * mat[7];
  out[2] =
      vec[0] * mat[8] + vec[1] * mat[9] + vec[2] * mat[10] + vec[3] * mat[11];
  out[3] =
      vec[0] * mat[12] + vec[1] * mat[13] + vec[2] * mat[14] + vec[3] * mat[15];
}

void p2m_mul_m4f(m4f a, m4f b, m4f out) {
  out[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
  out[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
  out[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
  out[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];
  out[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
  out[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
  out[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
  out[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];
  out[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
  out[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
  out[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
  out[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];
  out[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
  out[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
  out[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
  out[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

float p2m_len_v3f(v3f v) { return (float)sqrt(p2m_dot_v3f(v, v)); }

float p2m_dot_v3f(v3f a, v3f b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// basic vector operations
void p2m_norm_v3f(v3f v, v3f out) {
  float s = p2m_len_v3f(v);
  out[0] = v[0] / s;
  out[1] = v[1] / s;
  out[2] = v[2] / s;
}

void p2m_sub_v3f(v3f a, v3f b, v3f out) {
  out[0] = a[0] - b[0];
  out[1] = a[1] - b[1];
  out[2] = a[2] - b[2];
}

void p2m_add_v3f(v3f a, v3f b, v3f out) {
  out[0] = a[0] + b[0];
  out[1] = a[1] + b[1];
  out[2] = a[2] + b[2];
}

void p2m_scale_v3f(v3f a, float s, v3f out) {
  out[0] = a[0] * s;
  out[1] = a[1] * s;
  out[2] = a[2] * s;
}

void p2m_fixed_len_v3f(v3f a, float s, v3f out) {
  float len = p2m_len_v3f(a);
  p2m_scale_v3f(a, s / len, out);
}

void p2m_cross_v3f(v3f a, v3f b, v3f out) {
  out[0] = a[1] * b[2] - a[2] * b[1];
  out[1] = a[2] * b[0] - a[0] * b[2];
  out[2] = a[0] * b[1] - a[1] * b[0];
}

float p2m_dot_v4f(v4f a, v4f b) {
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

#if 0
static float far = 1000.0f;
static float near = 10.0f;
static float fov = 0.8377f;
#endif
void p2m_make_projection(float camera_z, m4f m) {
#if 1
  p2m_m4f_make_identity(m);
  // m[10] = -2.0f / (far - near);
  // m[11] = -1.0f * ((far + near) / (far - near));
  m[14] = -1.0f / camera_z;
#else
  float proj_s = 1.0f / ((float)tan(fov / 2));
  p2m_m4f_make_identity(m);
  m[0] = proj_s;
  m[5] = proj_s;
  m[10] = -far / (far - near);
  m[11] = -1;
  m[14] = -1.0f * (far * near) / (far - near);
  m[15] = 0;
#endif
}

void p2m_make_translation_m4f(m4f m, float x, float y, float z) {
  p2m_m4f_make_identity(m);
  m[3] = x;
  m[7] = y;
  m[11] = z;
}

void p2m_make_scale_m4f(m4f m, float sx, float sy, float sz) {
  p2m_m4f_make_identity(m);
  m[0] = sx;
  m[5] = sy;
  m[10] = sz;
  m[15] = 1;
}

void p2m_lookat(v3f eye, v3f center, v3f up, m4f out) {
  v3f z, x, y;
  p2m_sub_v3f(eye, center, z);
  p2m_norm_v3f(z, z);

  p2m_cross_v3f(up, z, x);
  p2m_norm_v3f(x, x);

  p2m_cross_v3f(z, x, y);
  p2m_norm_v3f(y, y);

  p2m_m4f_make_identity(out);

  for (int i = 0; i < 3; i++) {
    out[i] = x[i];
    out[4 + i] = y[i];
    out[8 + i] = z[i];
    out[4 * (i + 1) - 1] = -1 * center[i];
  }
}

// NOTE: flip Y
void p2m_viewport(float x, float y, float w, float h, m4f m) {
  p2m_m4f_make_identity(m);
  m[3] = x + w / 2.0f;
  m[7] = y + h / 2.0f;
  m[11] = 127.5f;
  m[0] = w / 2.0f;
  m[5] = h / -2.0f;
  m[10] = 127.5f;
}

void p2m_make_rot3d_x(float angle, m4f m) {
  float c = (float)cos(angle);
  float s = (float)sin(angle);
  p2m_m4f_make_identity(m);
  m[5] = c;
  m[6] = s;
  m[9] = -s;
  m[10] = c;
}

void p2m_make_rot3d_y(float angle, m4f m) {
  float c = (float)cos(angle);
  float s = (float)sin(angle);
  p2m_m4f_make_identity(m);
  m[0] = c;
  m[2] = -s;
  m[8] = s;
  m[10] = c;
}

void p2m_make_rot3d(float x, float y, float z, m4f m) {
  m4f ry;
  m4f rx;
  p2m_make_rot3d_x(x, rx);
  p2m_make_rot3d_y(y, ry);
  p2m_mul_m4f(rx, ry, m);
}
