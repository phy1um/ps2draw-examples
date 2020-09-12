
#include <kernel.h>
#include <dma_tags.h>
#include <gif_tags.h>
#include <gs_psm.h>
#include <gs_gp.h>
#include <dma.h>
#include <graph.h>
#include <draw.h>

#include <stdio.h>

#include "gs.h"

// --- Magic Numbers ? ---
//  from ps2sdk draw.c
#define START_OFFSET 2047.5625f
#define END_OFFSET 2048.5625f

#define BLENDING 0

// local functions
int gs_draw_rect_strips(drawbuf *b, struct colour *rgb, float x, float y, float w, float h);

// File state is a cheap hack for now
static framebuffer_t fb;
static zbuffer_t zbuf;
static int used_vram_bytes;

// Dumb buffer for small fixed dma transfers
static qword_t static_buffer[32];

/**
 * Setup GS and perform any initialization to start drawing
 */
int gs_init(struct gs_settings *cfg)
{
    dma_channel_initialize(DMA_CHANNEL_GIF, 0, 0);
    dma_channel_fast_waits(DMA_CHANNEL_GIF);

    // Allocate buffers
    fb.width = cfg->width;
    fb.height = cfg->height;
    fb.mask = 0;
    fb.psm = cfg->psm;
    fb.address = graph_vram_allocate(cfg->width, cfg->height, cfg->psm, GRAPH_ALIGN_PAGE);
    used_vram_bytes = graph_vram_size(cfg->width, cfg->height, cfg->psm, GRAPH_ALIGN_PAGE); 

    zbuf.enable = DRAW_ENABLE;
    zbuf.mask = 0;
    zbuf.method = ZTEST_METHOD_GREATER_EQUAL;
    zbuf.zsm = cfg->psmz;
    zbuf.address = graph_vram_allocate(cfg->width, cfg->height, cfg->psmz, GRAPH_ALIGN_PAGE);
    used_vram_bytes += graph_vram_size(cfg->width, cfg->height, cfg->psmz, GRAPH_ALIGN_PAGE);

    // Setup video mode and graphics
    int vmode = cfg->mode == GRAPH_MODE_AUTO ? graph_get_region() : cfg->mode;
    int gmode = cfg->interlaced ? GRAPH_MODE_INTERLACED : GRAPH_MODE_NONINTERLACED;

    graph_set_mode(gmode, vmode, GRAPH_MODE_INTERLACED, GRAPH_DISABLE);
    graph_set_screen(cfg->pos_x, cfg->pos_y, cfg->width, cfg->height);
    graph_set_bgcolor(0, 0, 0);
    graph_set_framebuffer_filtered(fb.address, fb.width, fb.psm, 0, 0);
    graph_enable_output();
//    graph_initialize(fb.address, fb.width, fb.height, fb.psm, 0, 0);

    printf("Setup GS [[ VMode=%d, Interlace=%d, Width=%d, Height=%d ]]\n",
            vmode, gmode, cfg->width, cfg->height);

    // Send init to GS
    qword_t *buf = static_buffer;
    qword_t *q = draw_setup_environment(buf, 0, &fb, &zbuf);
    q = draw_primitive_xyoffset(q, 0, 2048 - (cfg->width/2), 2048 - (cfg->height/2));
    q = draw_finish(q);
    dma_channel_send_normal(DMA_CHANNEL_GIF, buf, q - buf, 0, 0);
    dma_wait_fast();

    return 1;
}

/**
 * Poor abstraction, temporary
 */
int gs_get_fbaddr()
{
    return fb.address;
}

/**
 * Poor abstraction, temporary
 */
zbuffer_t *gs_get_zbuf()
{
    return &zbuf;
}

int gs_clear(drawbuf *b, struct colour *rgb)
{
    qword_t *qws = static_buffer;
    qws = draw_disable_tests(qws, 0, gs_get_zbuf());
    int halfw = ((float)fb.width) / 2.0f;
    int halfh = ((float)fb.height) / 2.0f;
    qws = draw_clear(qws,0,2048.0f-halfw,2048.0f-halfh,fb.width,fb.height,0x0,0x00,0x00);
    //qws = draw_enable_tests(qws, 0, gs_get_zbuf());
    dma_channel_send_normal(DMA_CHANNEL_GIF, static_buffer , qws - static_buffer, 0, 0);
    dma_wait_fast();
    return 1;
}

/**
 * Append commands to a drawbuffer to clear the screen
 *
 * Does not create a new DMA packet.
 *
int gs_clear(drawbuf *b, struct colour *rgb)
{
    // Disable tests - every pixel should be drawn regardless of depth etc
    giftag_begin(b, GIF_FLG_PACKED);
    giftag_push_register(b, GIF_REG_AD);
    giftag_packed_regs(b, GS_REG_TEST,
                                GS_SET_TEST(DRAW_ENABLE,ATEST_METHOD_NOTEQUAL,0x00,ATEST_KEEP_FRAMEBUFFER,
                                DRAW_DISABLE,DRAW_DISABLE,
                                DRAW_ENABLE,ZTEST_METHOD_ALLPASS));
    giftag_force_nloops(b, 1);
    giftag_end(b);

    // Force primitive override, draw flat shading
    giftag_begin(b, GIF_FLG_PACKED);
    giftag_push_register(b, GIF_REG_AD);
    giftag_packed_regs(b, GS_REG_PRMODECONT, GS_SET_PRMODECONT(PRIM_OVERRIDE_ENABLE));
    giftag_packed_regs(b, GS_REG_PRMODE, GS_SET_PRMODE(0,0,0,0,0,0,0,1));
    giftag_force_nloops(b, 2);
    giftag_end(b);

    // Fill the screen with a bunch of coloured rectangles
    gs_draw_rect_strips(b, rgb, 2048-(fb.width), 2048-(fb.height), fb.width, fb.height);

    // Disable primitive override
    giftag_begin(b, GIF_FLG_PACKED);
    giftag_push_register(b, GIF_REG_AD);
    giftag_packed_regs(b, GS_REG_PRMODECONT, GS_SET_PRMODECONT(PRIM_OVERRIDE_DISABLE));
    giftag_force_nloops(b, 1);
    giftag_end(b);

    // Re-enable pixel tests
    giftag_begin(b, GIF_FLG_PACKED);
    giftag_push_register(b, GIF_REG_AD);
    giftag_packed_regs(b, GS_REG_TEST,
                                GS_SET_TEST(DRAW_ENABLE,ATEST_METHOD_NOTEQUAL,0x00,ATEST_KEEP_FRAMEBUFFER,
                                DRAW_DISABLE,DRAW_DISABLE,
                                DRAW_ENABLE,zbuf.method));
    giftag_force_nloops(b, 1);
    giftag_end(b);

    return 1;
} */


#define AS_INT(f) *((int*)&f)
/**
 * Cheap translation of PS2SDK draw's version of this function to my
 * buffer abstraction.
 *
 * Does not create a new DMA packet
 */
int gs_draw_rect_strips(drawbuf *b, struct colour *rgb, float x, float y, float w, float h)
{
    // Floats to fixed point
    int fx0 = ftoi4(x);
    int fy0 = ftoi4(y+START_OFFSET);
    int fx1 = ftoi4(x+w);
    int fy1 = ftoi4(y+h+END_OFFSET);

    // Set PRIM and RGBAQ (fix colour rects)
    giftag_begin(b, GIF_FLG_PACKED);
    giftag_push_register(b, GIF_REG_AD);
    giftag_packed_regs(b, GS_REG_PRIM, GS_SET_PRIM(PRIM_SPRITE, 0,0,0, BLENDING, 0,0, 0, 0));
    giftag_packed_regs(b, GS_REG_RGBAQ, GS_SET_RGBAQ(rgb->r, rgb->g, rgb->b, rgb->a, AS_INT(rgb->q)));
    giftag_force_nloops(b, 2);
    giftag_end(b);
    // END PRIM and RGBAQ

    // XYZ data
    giftag_begin(b, GIF_FLG_REGLIST);
    // With reglist we can fit 2x more points over Packed
    giftag_push_register(b, GS_REG_XYZ2);
    giftag_push_register(b, GS_REG_XYZ2);

    while(fx0 < fx1) {
        giftag_push_data(b, GIF_SET_XYZ(fx0 + ftoi4(START_OFFSET), fy0, 0));
        fx0 += 496;
        if(fx0 >= fx1) {
            fx0 = fx1;
        }
        giftag_push_data(b, GIF_SET_XYZ(fx0 + ftoi4(END_OFFSET), fy1, 0));
        fx0 += 16;
    }
    // Calcualte how many qwords we wrote (nbytes % 4 should ALWAYS == 0 here)
    // We subtract one qword for the GIF tag
    int nbytes = (b->gs_packet_word_count - 4);
    // Unneeded but safe way to force us to go to the next boundry
    while(nbytes%4 != 0) nbytes++;
    nbytes /= 4;
    // Consider the case where no strips were written!
    giftag_force_nloops(b, nbytes >= 0 ? nbytes : 0);

    giftag_end(b);

    drawbuf_mark_last_gif_eop(b);
    // END XYZ data
    return 1;
}

/**
 * Add a GIF Tag to a buffer to end drawing
 *
 * Does not create a new DMA Packet
 */
int gs_draw_finish(drawbuf *b)
{
    giftag_begin(b, GIF_FLG_PACKED);
    giftag_push_register(b, GIF_REG_AD);
    giftag_packed_regs(b, GS_REG_FINISH, 1);
    giftag_force_nloops(b, 1);
    giftag_end(b);
    drawbuf_mark_last_gif_eop(b);
    return 1;
}

