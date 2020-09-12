
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

/**
 * Clear the screen
 * Command sent using a small static DMA buffer, does nto use drawbuf (for now?)
 */
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

