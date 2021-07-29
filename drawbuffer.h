#ifndef PS2_DRAWBUFFER_H
#define PS2_DRAWBUFFER_H

#include <string.h>

#include "inttypes.h"

struct gif_tag {
  uint64_t regs;
  int nreg;
  int nloop;
  int pre;
  int prim;
  int active;
  int type;
  int eop;
  uint64_t *tgt;
};

struct dma_chain_tag {
  uint32_t addr;
  int words;
  int pce;
  int type;
  int irq;
  uint64_t extra;
  int active;
  uint64_t *tgt;
};

struct draw_buffer {
  struct dma_chain_tag dmatag;
  struct gif_tag giftag;
  struct gif_tag *last_giftag;
  uint64_t *send_head;
  uint64_t *write_head;
  int buffer_len;
  int gs_packet_word_count;
};

typedef struct draw_buffer drawbuf;

/**
 * Clear the draw buffer, zero all data
 */
void drawbuf_begin(drawbuf *b);

/**
 * Cleanup ongoing tags.. (?)
 */
void drawbuf_end(drawbuf *b);

// ==========
// Make and Use DMA Tags

/**
 * Start a DMA CNT packet (copy quadwords following this tag)
 */
void drawbuf_start_cnt(drawbuf *b);

/**
 * Start a DMA REF packet (copy quadwords from a given address)
 */
void drawbuf_make_ref(drawbuf *b, int addr, int count);

/**
 * Set the extra doubleword that sits next to a DMA tag
 * (Used in GIF transfers maybe)
 */
void drawbuf_dma_set_extra(drawbuf *b, uint64_t extra);

/**
 * Close a DMA packet
 * Only used for CNT, REFs have no body local to the drawbuffer
 */
void drawbuf_dma_packet_end(drawbuf *b, int end);

// ==========
// Building GIF packets

/**
 * Start a GIF packet
 */
void giftag_begin(drawbuf *b, int gif_tag_type);

/**
 * Force set the registers of a GIF packet (not advised)
 * Manually pass nregs, the number of registers masked in this int
 */
void giftag_set_regs(drawbuf *b, uint64_t regs, int nregs);

/**
 * Push a register into the GIF packet's register format
 * Automatically counts nregs
 * You must do this 1-16 times!
 */
void giftag_push_register(drawbuf *b, uint64_t reg);

/**
 * End a GIF packet, update giftag fields
 */
void giftag_end(drawbuf *b);

/**
 * Push a dword of data to the GIF packet
 */
void giftag_push_data(drawbuf *b, uint64_t d);
void giftag_packed_regs(drawbuf *b, uint64_t reg, uint64_t value);

/**
 * Temporary workaround - DO NOT USE
 */
void giftag_force_nloops(drawbuf *b, int nloops);

void drawbuf_mark_last_dma_end(drawbuf *b);
void drawbuf_mark_last_gif_eop(drawbuf *b);

void drawbuf_submit_normal(drawbuf *b, int channel);
void drawbuf_print(drawbuf *b);

void giftag_packed_prim(drawbuf *b, uint64_t primdata);
void giftag_packed_rgbaq(drawbuf *db, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a);
void giftag_packed_xyz2(drawbuf *d, uint16_t x, uint16_t y, uint32_t z,
                        int adc);

#endif
