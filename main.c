
#include <dma.h>
#include <dma_tags.h>
#include <draw.h>
#include <draw_types.h>
#include <gif_tags.h>
#include <graph.h>
#include <gs_gp.h>
#include <gs_psm.h>

#include <malloc.h>
#include <stdio.h>

#include "drawbuffer.h"
#include "gs.h"
#include "inputs.h"
#include "inttypes.h"
#include "ps2math.h"
#include "log.h"

float tris[] = {0, 0, 0, 1, 0, 0, 0, 1, 0,

                1, 0, 0, 1, 1, 0, 0, 1, 0,

                0, 0, 0, 0, 0, 1, 0, 1, 0,

                0, 0, 1, 0, 1, 1, 0, 1, 0};

/*
void wait(unsigned long ms)
{
    clock_t start = clock();
    clock_t now;
    float diff = 0;
    do {
        now = clock();
        diff = (float) (now - start) / (float) CLOCKS_PER_SEC;
    }
    while(diff*1000 < ms);
}
*/

#define XOFF (2110 << 4)
#define YOFF (2030 << 4)
#define HSCALE 1.2
void putv3f(drawbuf *b, v3f p) {
  giftag_packed_xyz2(b, ftoi4(p[0] * HSCALE) + XOFF, ftoi4(p[1]) + YOFF, p[2],
                     0);
}

static float ry = 0;
static float rx = 0;
static float xx = 0;
static m4f rot;

void update() {
  sys_pad_poll();

  if (in_btn_held(DPAD_LEFT)) {
    ry -= 0.2f;
  }
  if (in_btn_held(DPAD_RIGHT)) {
    ry += 0.2f;
  }
  if (in_btn_held(DPAD_UP)) {
    rx -= 0.2f;
  }
  if (in_btn_held(DPAD_DOWN)) {
    rx += 0.2f;
  }
  if (in_btn_held(BTN_R1)) {
    xx += 3.2f;
  }
  if (in_btn_held(BTN_L1)) {
    xx -= 3.2f;
  }
  p2m_make_rot3d(rx, ry, 0, rot);
}

void transform(v3f from, v3f to) {
  // info("transforming [%f %f %f]", from[0], from[1], from[2]);
  v4f w;
  setv4(w, 0, 0, 0, 1);
  copyv3fto(from, w);
  // logv3f("Before: ", w);
  v4f out;
  p2m_mul_m4f_v4f(rot, w, out);
  // logv3f("Rotated: ", out);

  out[0] *= 100;
  out[1] *= 100;

  out[0] += xx;

  to[0] = out[0] / out[3];
  to[1] = out[1] / out[3];
  to[2] = out[2] / out[3];

  // info("transformed to -> [%f %f %f]", to[0], to[1], to[2]);
}

void draw(drawbuf *b) {
  int ntris = 1;
  dma_wait_fast();

  drawbuf_begin(b);
  giftag_begin(b, GIF_FLG_PACKED);
  giftag_push_register(b, GIF_REG_AD);
  // Shaded traingle primitive
  giftag_packed_regs(b, GS_REG_PRIM, GS_SET_PRIM(3, 1, 0, 0, 0, 0, 0, 0, 0));
  giftag_force_nloops(b, 1);
  giftag_end(b);

  giftag_begin(b, GIF_FLG_PACKED);
  // setup registers
  giftag_push_register(b, GS_REG_XYZ2);
  giftag_push_register(b, GS_REG_RGBAQ);
  giftag_push_register(b, GS_REG_XYZ2);
  giftag_push_register(b, GS_REG_RGBAQ);
  giftag_push_register(b, GS_REG_XYZ2);
  giftag_push_register(b, GS_REG_RGBAQ);

  for (int i = 0; i < ntris; i++) {
    for (int j = 0; j < 3; j++) {
      v3f v;
      transform(tris + 9 * i + 3 * j, v);
      v3f uv;
      copyv3fto(v, uv);

      uv[0] += 320;
      uv[1] += 240;
      uv[0] /= 640.0f;
      uv[1] /= 480.0f;

      giftag_packed_rgbaq(b, uv[0] * 255, 0, uv[1] * 255, 0x80);
      // giftag_packed_rgbaq(b, 255, 0, 0, 0x80);
      putv3f(b, v);
    }
  }

  giftag_force_nloops(b, ntris);
  giftag_end(b);

  // drawbuf_mark_last_gif_eop(b);

  gs_draw_finish(b);

  drawbuf_print(b);

  // DMA send
  // dma_channel_send_normal(DMA_CHANNEL_GIF, head,q - head, 0, 0);
  drawbuf_submit_normal(b, DMA_CHANNEL_GIF);
}

int main() {
  printf("Startup!\n");
  printf("Int sizes are: u32=%u, u64=%u, qword=%u\n", sizeof(uint32_t),
         sizeof(uint64_t), QWORD_SIZE);
  struct gs_settings cfg = {.pos_x = 0,
                            .pos_y = 0,
                            .width = 664,
                            .height = 448,
                            .psm = GS_PSM_32,
                            .psmz = GS_PSMZ_16S,
                            .mode = GRAPH_MODE_NTSC,
                            .interlaced = 0};

  sys_init_input();

  drawbuf dbuf;
  dbuf.send_head = memalign(64, 3000);
  drawbuf *b = &dbuf;

  gs_init(&cfg);

  p2m_make_rot3d(0, ry, 0, rot);

  while (1) {
    in_frame_start();

    gs_clear(0, 0);

    draw(b);

    // Wait until the drawing is finished.
    draw_wait_finish();

    update();

    // Now initiate vsync.
    graph_wait_vsync();
  }

  while (1)
    continue;
}
