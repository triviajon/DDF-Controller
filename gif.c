#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define SUCC_OUT 0
#define ERROR_OUT -1

#define DEBUG 1
#define DEBUG_CT 1  /* Display color table data */

#define DISPOSAL_NONE 0  /* No disposal assigned */
#define DISPOSAL_RETAIN 1  /* Do not dispose current frame */
#define DISPOSAL_BG 2  /* Replace current frame with background color */
#define DISPOSAL_PREV 3  /* Revert to previous frame */

#define FRAME_ALLOC_BLOCK 8

#define CT_GLOBAL 0
#define CT_LOCAL 1

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

    /* DISPOSAL_RETAIN - Keep transparent pixels
       Any other disposal type - Replace transparent pixels with background color
       Any pixels outside frame boundaries become transparent */
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

    struct frame *frames;
    uint16_t num_frames;
    uint16_t frame_capacity;
};

uint16_t combine_bytes(uint8_t lsb, uint8_t msb) {
    return ((uint16_t) lsb) | (((uint16_t) msb) << 8);
}

void gif_init(struct gif *gif) {
    gif->num_frames = 0;
    gif->frame_capacity = 0;
}

void gif_load_ct(struct gif *gif, uint8_t max_ct_color, uint8_t ct_type, FILE *file) {    
    uint8_t i, j;
    uint16_t ct_bytes;
    uint16_t ct_byte;
    uint8_t *ct;

    ct_bytes = (max_ct_color + 1) * 3;
    ct_byte = 0;
    ct = (uint8_t *) malloc(ct_bytes);
    fread(ct, 1, ct_bytes, file);

    #if DEBUG_CT
        if (ct_type == CT_LOCAL) {
            printf("Local color table:\n");
        }
        else {
            printf("Global color table:\n");
        }
    #endif

    i = 0;
    while (1) {
        /* Loop for i = 0 to max_ct_color
           max_ct_color can be 255, so need to be careful of overflow */

        #if DEBUG_CT
            printf("    Row %d: %d %d %d\n", 
                i, ct[ct_byte], ct[ct_byte + 1], ct[ct_byte + 2]
            );
        #endif

        for (j = 0; j < 3; ++j) {
            if (ct_type == CT_LOCAL) {
                gif->frames[gif->num_frames].lct[i][j] = ct[ct_byte];
            }
            else {
                gif->gct[i][j] = ct[ct_byte];
            }
            ++ct_byte;
        }

        if (i == max_ct_color) {
            break;
        }
        ++i;
    }

    free(ct);
}

void gif_decode(struct gif *gif, uint8_t *frame_codes, uint16_t code_bytes) {
    /* Decode LZW data */
}

int gif_load_frame(struct gif *gif, struct gce *gce, uint8_t *buffer, FILE *file) { 
    /* Load frame with (optional) GCE data
       Buffer must be able to contain at least 9 bytes */

    struct id id;

    uint8_t *frame_codes = 0;
    uint32_t code_bytes = 0;
    uint8_t min_code_size;

    if (gif->frame_capacity == gif->num_frames) {
        if (!gif->num_frames) {
            /* No memory allocated yet for frames */
            gif->frames = (struct frame *) malloc(
                FRAME_ALLOC_BLOCK * sizeof(struct frame)
            );
        }
        else {
            /* Extend frame memory */
            gif->frames = (struct frame *) realloc(
                gif->frames,
                (gif->num_frames + FRAME_ALLOC_BLOCK) * sizeof(struct frame)
            );
        }
        gif->frame_capacity += FRAME_ALLOC_BLOCK;
    }

    /* Image desriptor */
    fread(buffer, 1, 9, file);
    id.img_left = combine_bytes(buffer[0], buffer[1]);
    id.img_top = combine_bytes(buffer[2], buffer[3]);
    id.img_w = combine_bytes(buffer[4], buffer[5]);
    id.img_h = combine_bytes(buffer[6], buffer[7]);
    id.is_interlaced = (buffer[8] >> 6) & 1U;
    if (id.is_interlaced) {
        printf("Warning: GIF is interlaced, will be ignored\n");
    }
    gif->frames[gif->num_frames].has_lct = (buffer[8] >> 7) & 1U;

    #if DEBUG
        printf("Image descriptor:\n");
        printf("    Image left: %d\n", id.img_left);
        printf("    Image top: %d\n", id.img_top);
        printf("    Image width: %d\n", id.img_w);
        printf("    Image height: %d\n", id.img_h);
        printf("    LCT flag: %d\n", gif->frames[gif->num_frames].has_lct);
        printf("    Interlace flag: %d\n", id.is_interlaced);
        printf("    Sort flag: %d\n", (buffer[8] >> 5) & 1U);
        printf("    LCT size: %d\n", buffer[8] & 0b111U);
    #endif

    /* Local color table */
    if (gif->frames[gif->num_frames].has_lct) {
        gif_load_ct(gif, (1U << ((buffer[8] & 0b111U) + 1)) - 1, CT_LOCAL, file);
    }
    else if (!gif->has_gct) {
        printf("Error: Frame has no global or local color table\n");
        return ERROR_OUT;
    }

    /* Image data */
    fread(buffer, 1, 1, file);
    min_code_size = buffer[0];
    while (1) {
        fread(buffer, 1, 1, file);
        if (!buffer[0]) {
            break;
        }

        code_bytes += buffer[0];

        if (!frame_codes) {
            frame_codes = (uint8_t *) malloc(code_bytes);
        }
        else {
            frame_codes = (uint8_t *) realloc(frame_codes, code_bytes);
        }

        fread(frame_codes + code_bytes - buffer[0], 1, buffer[0], file);
    }

    gif_decode(gif, frame_codes, code_bytes);
    free(frame_codes);

    #if DEBUG
        printf("Finished loading frame %d\n", gif->num_frames);
    #endif

    ++gif->num_frames;

    return SUCC_OUT;
}

void gif_squeeze_frames(struct gif *gif) {
    /* Deallocate unused frame memory */
    gif->frames = (struct frame *) realloc(
        gif->frames, gif->num_frames * sizeof(struct frame)
    );
}

int gif_load(struct gif *gif, const char *filename) {
    FILE *file;
    uint8_t buffer[9];

    struct gce gce;
    uint8_t frame_has_gce = 0;

    file = fopen(filename, "rb");

    /* Verify header signature */
    fread(buffer, 1, 3, file);
    if (!(buffer[0] == 'G' && buffer[1] == 'I' && buffer[2] == 'F')) {
        printf("Error: Invalid GIF file signature\n");
        return ERROR_OUT;
    }

    fseek(file, 3, SEEK_CUR);  /* Skip version */
    
    /* Logical screen descriptor */
    fread(buffer, 1, 7, file);
    gif->w = combine_bytes(buffer[0], buffer[1]);
    gif->h = combine_bytes(buffer[2], buffer[3]);
    gif->has_gct = (buffer[4] >> 7) & 1U;
    gif->bg_index = buffer[5];

    #if DEBUG
        printf("Logical screen descriptor:\n");
        printf("    Canvas width: %d\n", gif->w);
        printf("    Canvas height: %d\n", gif->h);
        printf("    GCT flag: %d\n", gif->has_gct);
        printf("    Color resolution: %d\n", (buffer[4] >> 4) & 0b111U);
        printf("    Sort flag: %d\n", (buffer[4] >> 3) & 1U);
        printf("    GCT size: %d\n", buffer[4] & 0b111U);
        printf("    Background color index: %d\n", gif->bg_index);
        printf("    Pixel aspect ratio: %d\n", buffer[6]);
    #endif

    if (gif->has_gct) {
        gif_load_ct(gif, (1U << ((buffer[4] & 0b111U) + 1)) - 1, CT_GLOBAL, file);
    }

    while (1) {
        if (!fread(buffer, 1, 1, file)) {
            break;
        }
        if (buffer[0] == 0x21) {
            /* Extension block */
            fread(buffer, 1, 1, file);  /* Get extension label */
            if (buffer[0] == 0xF9) {
                /* Graphic control extension */

                #if DEBUG
                    printf("Graphic control extension:\n");
                #endif

                fread(buffer, 1, 1, file);  /* Get block size */
                fread(buffer, 1, buffer[0] + 1, file);  /* Get block data */
                gce.has_transparency = buffer[0] & 1U;
                gce.disposal = (buffer[0] >> 2) & 0b111U;
                if (gce.disposal == DISPOSAL_PREV) {
                    printf("Warning: DISPOSAL_PREV disposal type unsupported\n");
                }
                else if (gce.disposal >= 4) {
                    printf("Warning: Unknown disposal type %d\n", gce.disposal);
                }
                gce.delay = combine_bytes(buffer[1], buffer[2]); 
                gce.transparent_index = buffer[3];

                #if DEBUG
                    printf("    Disposal method: %d\n", gce.disposal);
                    printf("    User input flag: %d\n", (buffer[0] >> 1) & 1U);
                    printf("    Transparent color flag: %d\n", gce.has_transparency);
                    printf("    Delay time: %d\n", gce.delay);
                    printf("    Transparent color index: %d\n", gce.transparent_index);
                #endif

                frame_has_gce = 1;
            }
            else {
                /* Any other extension */
                printf("Warning: Skipping extension with label 0x%X\n", buffer[0]);
                
                /* Skip all data sub-blocks */
                while (1) {
                    fread(buffer, 1, 1, file);
                    if (!buffer[0]) {
                        break;
                    }
                    fseek(file, buffer[0], SEEK_CUR);
                }

                frame_has_gce = 0;
            }
        }
        else if (buffer[0] == 0x2C) {
            /* Image descriptor reached, load a full frame */
            if (gif_load_frame(gif, frame_has_gce ? &gce : 0, buffer, file) == ERROR_OUT) {
                return ERROR_OUT;
            }
            frame_has_gce = 0;
        }
        else if (buffer[0] == 0x3B) {
            /* Trailer */
            break;
        }
        else {
            printf("Error: Unknown block start byte 0x%X\n", buffer[0]);
            return ERROR_OUT;
        }
    }

    gif_squeeze_frames(gif);

    #if DEBUG
        printf("Loaded %d frames\n", gif->num_frames);
    #endif

    return SUCC_OUT;
}

void gif_free(struct gif *gif) {
    /* TODO - Disabled for testing */

    /*
    uint16_t i;
    for (i = 0; i < gif->num_frames; ++i) {
        free(gif->frames[i].ct_indices);
    }
    */

    free(gif->frames);
}

int main() {
    struct gif gif;
    gif_init(&gif);
    if (gif_load(&gif, "img/sample5.gif") == ERROR_OUT) {
        return ERROR_OUT;
    }
    gif_free(&gif);
    return SUCC_OUT;
}

