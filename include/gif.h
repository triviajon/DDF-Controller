#ifndef GIF_H
#define GIF_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "global_defines.h"
#include "util.h"
#include "dyn_arr.h"

/* Graphic control extension */
struct gce {
    uint8_t disposal;
    uint8_t has_transparency;
    uint16_t delay;
    uint8_t transparent_index;
};

/* Image descriptor */
struct id {
    uint16_t img_left;
    uint16_t img_top;
    uint16_t img_w;
    uint16_t img_h;
    uint8_t is_interlaced;
};

struct frame {
    uint8_t lct[256][3];
    uint8_t has_lct;

    uint8_t *ct_indices;  /* Size: canvas_w * canvas_h */

    uint16_t delay;

    /* Pixels outside frame bounds assigned transparent_index
       DISPOSAL_RETAIN - Retain transparent_index
       Any other disposal type - Replace transparent_index with bg_index */
    uint8_t disposal;
    uint8_t has_transparency;
    uint8_t transparent_index;
};

struct gif {
    uint16_t w;
    uint16_t h;

    uint8_t gct[256][3];
    uint8_t has_gct;

    uint8_t bg_index;

    struct dyn_arr frames;
};

void gif_init(struct gif *gif);
void gif_load_ct(struct gif *gif, uint8_t max_ct_color, struct frame *frame, FILE *file);
void gif_decode(
    struct gif *gif,
    struct id *id,
    struct frame *frame,
    uint8_t *frame_codes, uint32_t code_bytes,
    uint8_t min_code_size
);
int gif_load_frame(struct gif *gif, struct gce *gce, uint8_t *buffer, FILE *file);
int gif_load(struct gif *gif, const char *filename);
void gif_free(struct gif *gif);

#endif

