
#ifndef PS2_VUTEST_GS_H
#define PS2_VUTEST_GS_H

#include "drawbuffer.h"

struct gs_settings {
  unsigned int pos_x;
  unsigned int pos_y;
  unsigned int width;
  unsigned int height;
  int psm;
  int psmz;
  int mode;
  int interlaced;
};

struct colour {
  char r;
  char g;
  char b;
  char a;
  float q;
};

int gs_init(struct gs_settings *cfg);
int gs_clear(drawbuf *b, struct colour *rgbaq);

int gs_draw_finish(drawbuf *b);

int gs_get_fbaddr();

#endif
