
#include "drawbuffer.h"
#include "log.h"
#include <stdio.h>

#ifdef _EE
#include <dma.h>
#endif

#define SHIFT_AS_64(x, b) (((uint64_t)x) << b)

#define DMA_TYPE_REFE 0
#define DMA_TYPE_CNT 1
#define DMA_TYPE_NEXT 2
#define DMA_TYPE_REF 3
#define DMA_TYPE_REFS 4
#define DMA_TYPE_CALL 5
#define DMA_TYPE_RET 6
#define DMA_TYPE_END 7

void update_dma_tag_meta(drawbuf *b);

/**
 * Clear the draw buffer, zero all data
 */
void drawbuf_begin(drawbuf *b) {
  b->write_head = b->send_head;
  memset(&b->dmatag, 0, sizeof(struct dma_chain_tag));
  memset(&b->giftag, 0, sizeof(struct gif_tag));
  memset(b->send_head, 0, b->buffer_len);
}

/**
 * Cleanup ongoing tags.. (?)
 */
void drawbuf_end(drawbuf *b) {
  if (b->dmatag.active == 1) {
    warn("ending drawbuffer while DMATag still active");
  }
  if (b->giftag.active == 1) {
    warn("ending drawbuffer while GIFTag still active");
  }
  return;
}

void drawbuf_mark_last_gif_eop(drawbuf *b) {
  info("marking GIFTag EOP");
  if (b->giftag.tgt) {
    *(b->giftag.tgt) |= (1 << 15);
  }
}

// ==========
// Make and Use DMA Tags

/**
 * Start a DMA CNT packet (copy quadwords following this tag)
 */
void drawbuf_start_cnt(drawbuf *b) {
  info("starting DMA CNT packet");
  // clear dmatag
  memset(&b->dmatag, 0, sizeof(struct dma_chain_tag));
  b->dmatag.active = 1;
  // this tag starts at the current head
  b->dmatag.tgt = b->write_head;
  // advance head to make room for the dma tag
  b->write_head += 2;
  // CNTs zero word count and address - address will be ignored
  b->dmatag.type = DMA_TYPE_CNT;
}

/**
 * Start a DMA REF packet (copy quadwords from a given address)
 */
void drawbuf_make_ref(drawbuf *b, int addr, int count) {
  info("starting DMA REF packet");
  // clear dmatag
  memset(&b->dmatag, 0, sizeof(struct dma_chain_tag));
  b->dmatag.active = 1;
  // this tag starts at the current head
  b->dmatag.tgt = b->write_head;
  // advance head to make room for the dma tag
  b->write_head += 2;
  // REF uses a given address and count
  b->dmatag.type = DMA_TYPE_REF;
  b->dmatag.addr = addr;
  b->dmatag.words = count * 4;
  drawbuf_dma_packet_end(b, 0);
}

/**
 * Set the extra doubleword that sits next to a DMA tag
 * (Used in GIF transfers maybe)
 */
void drawbuf_dma_set_extra(drawbuf *b, uint64_t extra) {
  b->dmatag.extra = extra;
}

/**
 * Close a DMA packet
 * Only used for CNT, REFs have no body local to the drawbuffer
 */
void drawbuf_dma_packet_end(drawbuf *b, int end) {
  info("closing DMA packet");
  if (b->dmatag.active) {
    update_dma_tag_meta(b);
    // We are no longer building a dmatag, zero the pointer
    b->dmatag.active = 0;
  }
}

void drawbuf_mark_last_dma_end(drawbuf *b) {
  if (b->dmatag.type == DMA_TYPE_CNT) {
    info("marking last DMA packet as END");
    b->dmatag.type = DMA_TYPE_END;
  } else if (b->dmatag.type == DMA_TYPE_REF) {
    info("marking last DMA packet as REFE (end of REF chain)");
    b->dmatag.type = DMA_TYPE_REFE;
  }
  update_dma_tag_meta(b);
}

void update_dma_tag_meta(drawbuf *b) {
  info("committing DMATag header to buffer");
  uint64_t *h = b->dmatag.tgt;
  while (b->dmatag.words % 4 != 0) {
    b->dmatag.words++;
  }
  int qwords = b->dmatag.words / 4;
  // Set the DMA tag
  log_dbg("DMATag: words=%d, PCE=%d, TYPE=%d, ADDR=%lu\n", b->dmatag.words,
          b->dmatag.pce, b->dmatag.type, b->dmatag.addr);
  *h = (qwords & 0xffff) | ((b->dmatag.pce & 0x3) << 26) |
       ((b->dmatag.type & 0x7) << 28) | SHIFT_AS_64(b->dmatag.addr, 32);
  *(h + 1) = b->dmatag.extra;
}

// ==========
// Building GIF packets

/**
 * Start a GIF packet
 */
void giftag_begin(drawbuf *b, int gif_tag_type) {
  info("starting GIFTag");
  // Clear GIF tag
  memset(&b->giftag, 0, sizeof(struct gif_tag));
  b->giftag.active = 1;
  // Save position for building GIF tag
  b->giftag.tgt = b->write_head;
  // Advance head to make space for GIF tag
  b->write_head += 2;
  // Clear GIF tag state
  b->gs_packet_word_count = 4;
  b->giftag.nloop = 0;
  b->giftag.nreg = 0;
}

/**
 * Force set the registers of a GIF packet (not advised)
 * Manually pass nregs, the number of registers masked in this int
 */
void giftag_set_regs(drawbuf *b, uint64_t regs, int nregs) {
  if (b->giftag.active) {
    b->giftag.regs = regs;
    b->giftag.nreg = nregs;
  }
}

/**
 * Push a register into the GIF packet's register format
 * Automatically counts nregs
 * You must do this 1-16 times!
 */
void giftag_push_register(drawbuf *b, uint64_t reg) {
  if (b->giftag.active) {
    b->giftag.regs = b->giftag.regs << 4 | (reg & 0xf);
    b->giftag.nreg += 1;
  }
}

/**
 * End a GIF packet, update giftag fields
 */
void giftag_end(drawbuf *b) {
  if (b->giftag.active) {
    info("committing GIFTag header to buffer");
    // Update flags with packet size
    uint64_t *h = b->giftag.tgt;
    *h = (b->giftag.nloop & 0x7fff) | ((b->giftag.eop & 1) << 15) |
         SHIFT_AS_64(b->giftag.pre & 0x1, 46) |
         SHIFT_AS_64(b->giftag.prim & 0x7ff, 47) |
         SHIFT_AS_64(b->giftag.type & 0x3, 58) |
         SHIFT_AS_64(b->giftag.nreg & 0xf, 60);
    *(h + 1) = b->giftag.regs;
    // Update size of overall DMA packet (assuming CNT)
    b->dmatag.words += b->gs_packet_word_count;
    b->gs_packet_word_count = 0;
    b->giftag.active = 0;
  }
}

/**
 * Push a dword of data to the GIF packet
 */
void giftag_push_data(drawbuf *b, uint64_t d) {
  if (b->giftag.active) {
    *(b->write_head) = d;
    b->write_head++;
    b->gs_packet_word_count += 2;
  }
}

void giftag_packed_prim(drawbuf *b, uint64_t primdata) {
  giftag_push_data(b, primdata);
  giftag_push_data(b, 0);
}

void giftag_packed_rgbaq(drawbuf *db, unsigned char r, unsigned char g,
                         unsigned char b, unsigned char a) {
  giftag_push_data(db, ((uint64_t)r) | (((uint64_t)g) << 32));
  giftag_push_data(db, ((uint64_t)b) | (((uint64_t)a) << 32));
}

void giftag_packed_xyz2(drawbuf *d, uint16_t x, uint16_t y, uint32_t z,
                        int adc) {
  giftag_push_data(d, ((uint64_t)x) | SHIFT_AS_64(y, 32));
  giftag_push_data(d, ((uint64_t)z) << 4 | SHIFT_AS_64(adc & 1, 47));
}

void giftag_packed_regs(drawbuf *b, uint64_t reg, uint64_t value) {
  giftag_push_data(b, value);
  giftag_push_data(b, reg);
  b->giftag.nloop += 2;
}

/**
 * Temporary workaround - DO NOT USE
 */
void giftag_force_nloops(drawbuf *b, int nloops) {
  if (b->giftag.active) {
    info("forcing GIFTag NLOOPS = %d", nloops);
    b->giftag.nloop = nloops;
  }
}

int drawbuf_size(drawbuf *b) {
  int diff = b->write_head - b->send_head;
  while (diff % 2 != 0)
    diff++;
  return (int)diff;
}

void drawbuf_submit_normal(drawbuf *b, int channel) {
  int len = drawbuf_size(b);
  dma_channel_send_normal(channel, b->send_head, len / 2, 0, 0);
  dma_wait_fast();
}

void drawbuf_print(drawbuf *b) {
  int len = drawbuf_size(b);
  info("Printing buffer of size %d", len);
  int qws = len / 2;
  for (int i = 0; i < qws; i++) {
    info(" %d) %016llx %016llx", i, b->send_head[2 * i],
         b->send_head[2 * i + 1]);
  }
}
