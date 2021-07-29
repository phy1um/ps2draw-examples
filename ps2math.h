
#ifndef PS2MATH_H
#define PS2MATH_H

#define logm4f(msg, model)                                                     \
  printf("%s [ \n[%0.2f,%0.2f,%0.2f,%0.2f] \n[%0.2f,%0.2f,%0.2f,%0.2f] "       \
         "\n[%0.2f,%0.2f,%0.2f,%0.2f] \n[%0.2f,%0.2f,%0.2f,%0.2f ]",           \
         msg, model[0], model[1], model[2], model[3], model[4], model[5],      \
         model[6], model[7], model[8], model[9], model[10], model[11],         \
         model[12], model[13], model[14], model[15])

#define logv3f(msg, v) printf("%s [%0.2f %0.2f %0.2f]\n", msg, v[0], v[1], v[2])

typedef float m4f[16];
typedef float v3f[3];
typedef float v4f[4];

#define setv4(v, x, y, z, w)                                                   \
  do {                                                                         \
    v[0] = x;                                                                  \
    v[1] = y;                                                                  \
    v[2] = z;                                                                  \
    v[3] = w;                                                                  \
  } while (0)
#define setv3(a, x, y, z)                                                      \
  do {                                                                         \
    a[0] = x;                                                                  \
    a[1] = y;                                                                  \
    a[2] = z;                                                                  \
  } while (0)

#define copyv3fto(a, b)                                                        \
  do {                                                                         \
    b[0] = a[0];                                                               \
    b[1] = a[1];                                                               \
    b[2] = a[2];                                                               \
  } while (0)

void p2m_m4f_make_identity(m4f mat);
void p2m_mul_m4f_v4f(m4f mat, v4f vec, v4f out);
void p2m_mul_m4f(m4f, m4f, m4f);

float p2m_dot_v4f(v4f a, v4f b);
float p2m_len_v3f(v3f v);
float p2m_dot_v3f(v3f, v3f);
void p2m_norm_v3f(v3f v, v3f out);
void p2m_sub_v3f(v3f a, v3f b, v3f out);
void p2m_add_v3f(v3f a, v3f b, v3f out);
void p2m_scale_v3f(v3f a, float s, v3f out);
void p2m_fixed_len_v3f(v3f a, float l, v3f out);
void p2m_cross_v3f(v3f a, v3f b, v3f out);

void p2m_make_projection(float, m4f);
void p2m_make_translation_m4f(m4f, float, float, float);
void p2m_make_scale_m4f(m4f, float, float, float);
void p2m_lookat(v3f eye, v3f center, v3f up, m4f out);
void p2m_viewport(float x, float y, float w, float h, m4f m);

void p2m_make_rot3d(float x, float y, float z, m4f m);

#define clampz(x)                                                              \
  do {                                                                         \
    x = x < 0 ? 0 : x;                                                         \
  } while (0)

#endif
