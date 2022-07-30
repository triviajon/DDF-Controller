#include "gif.h"

#define DISPOSAL_NONE 0  /* No disposal assigned */
#define DISPOSAL_RETAIN 1  /* Do not dispose current frame */
#define DISPOSAL_BG 2  /* Replace current frame with background color */
#define DISPOSAL_PREV 3  /* Revert to previous frame */

void gif_init(struct gif *gif) {
    dyn_arr_init(&gif->frames, 8, sizeof(struct frame));
}

void gif_load_ct(struct gif *gif, uint8_t max_ct_color, struct frame *frame, FILE *file) {    
    /* Load a global color table if frame is null, or a local color table otherwise */

    uint8_t i, j;
    uint16_t ct_bytes;
    uint16_t ct_byte;
    uint8_t *ct;

    ct_bytes = (max_ct_color + 1) * 3;
    ct_byte = 0;
    ct = (uint8_t *) malloc(ct_bytes);
    fread(ct, 1, ct_bytes, file);

    if (frame) {
        frame->max_ct_color = max_ct_color;
        #if DEBUG_CT
            printf("Local color table:\n");
        #endif
    }
    else {
        gif->max_gct_color = max_ct_color;
        #if DEBUG_CT
            printf("Global color table:\n");
        #endif
    }

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
            if (frame) {
                frame->lct[i][j] = ct[ct_byte];
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

void gif_init_code_table(struct dyn_arr *code_table, struct frame *frame) {
    /* Initialize code table with gct or lct indices, clear code, EOI code */

    struct code_table_entry entry;
    uint8_t i = 0;

    while (1) {
        entry.indices = (uint8_t *) malloc(1);
        entry.length = 1;
        entry.indices[0] = i;
        dyn_arr_append(code_table, &entry);

        if (i == frame->max_ct_color) {
            break;
        }

        ++i;
    }

    entry.indices = 0;
    entry.length = 0;

    dyn_arr_append(code_table, &entry);  /* Clear code */
    dyn_arr_append(code_table, &entry);  /* EOI code */
}

void gif_free_code_table(struct dyn_arr *code_table) {
    struct code_table_entry *entry;
    uint16_t i;

    for (i = 0; i < code_table->length; ++i) {
        entry = (struct code_table_entry *) dyn_arr_get(code_table, i);
        free(entry->indices);
    }

    dyn_arr_free(code_table);
}

uint8_t gif_increment_decode(
    struct gif *gif,
    uint16_t *frame_row,
    uint16_t *frame_col,
    uint32_t *index_counter
) {
    /* Increment decode counters, return 0 if at end */
    ++(*index_counter);
    ++(*frame_col);
    if (*frame_col == gif->w) {
        *frame_col = 0;
        ++(*frame_row);
        if (*frame_row == gif->h) {
            *frame_row = 0;
            return 0;
        }
    }
    return 1;
}

void gif_decode(
    struct gif *gif,
    struct id *id,
    struct frame *frame,
    uint8_t *frame_codes, uint32_t code_bytes,
    uint8_t min_code_size
) {
    /* Decode LZW data */

    uint32_t i;
    uint8_t j;
    uint16_t k;
    uint16_t code = 0;
    uint16_t prev_code = 0;
    uint16_t code_size = min_code_size + 1;
    uint8_t code_bit_index = 0;

    /* Counters to track position on canvas */
    uint16_t frame_row = 0;
    uint16_t frame_col = 0;
    uint32_t index_counter = 0;

    uint16_t outside_bounds_index;  /* Color of pixels outside frame */ 

    struct dyn_arr code_table;
    struct code_table_entry *prev_entry;
    struct code_table_entry *rd_entry;
    struct code_table_entry wr_entry;
    uint8_t has_read_init_code = 0;

    dyn_arr_init(&code_table, 16, sizeof(struct code_table_entry));
    frame->ct_indices = (uint8_t *) malloc(gif->w * gif->h);

    if (frame->disposal == DISPOSAL_RETAIN) {
        outside_bounds_index = frame->transparent_index;
    }
    else {
        outside_bounds_index = gif->bg_index;
    }

    for (i = 0; i < code_bytes; ++i) {
        for (j = 0; j < 8; ++j) {
            /* Grab code_size bits from frame_codes one bit at a time */
            code |= ((frame_codes[i] >> j) & 1U) << code_bit_index;
            ++code_bit_index;
            if (code_bit_index == code_size) {
                /* Finished reading single code */
 
                while (frame_row < id->img_top || frame_row >= id->img_top + id->img_h ||
                    frame_col < id->img_left || frame_col >= id->img_left + id->img_w) {
                    /* Outside bounds of frame on canvas */
                
                    if (frame->disposal == DISPOSAL_RETAIN) {
                        frame->ct_indices[index_counter] = outside_bounds_index;
                    }

                    if (!gif_increment_decode(gif, &frame_row, &frame_col, &index_counter)) {
                        goto gif_decode_end;
                    }
                }

                if (code == ((uint16_t) frame->max_ct_color) + 1) {
                    /* Clear code */
                    gif_free_code_table(&code_table);
                    gif_init_code_table(&code_table, frame);
                    code_size = min_code_size + 1;
                    has_read_init_code = 0;
                }
                else if (code == ((uint16_t) frame->max_ct_color) + 2) {
                    /* EOI code */
                    goto gif_decode_end;
                }
                else {
                    if (code < code_table.length) {
                        /* Code is in code table */
                        rd_entry = (struct code_table_entry *) dyn_arr_get(&code_table, code);
                    }

                    if (has_read_init_code && code_table.length) {
                        /* Only update code table if this is not the first code read */
                        prev_entry = (struct code_table_entry *) dyn_arr_get(&code_table, prev_code);
                        wr_entry.length = 1 + prev_entry->length; 
                        wr_entry.indices = malloc(wr_entry.length);
                        memcpy(wr_entry.indices, prev_entry->indices, prev_entry->length);

                        if (code_table.length == (1U << code_size) - 1 && code_size < 12) {
                            ++code_size;
                        }

                        if (code < code_table.length) {
                            wr_entry.indices[prev_entry->length] = rd_entry->indices[0];
                            dyn_arr_append(&code_table, &wr_entry);

                            /* rd_entry may have been modified by realloc in dyn_arr_append */
                            rd_entry = (struct code_table_entry *) dyn_arr_get(&code_table, code);
                        }
                        else {
                            wr_entry.indices[prev_entry->length] = prev_entry->indices[0];
                            rd_entry = (struct code_table_entry *) dyn_arr_append(&code_table, &wr_entry);
                        }
                    }

                    /* Write to ct_indices */
                    memcpy(frame->ct_indices + index_counter, rd_entry->indices, rd_entry->length);
                    for (k = 0; k < rd_entry->length; ++k) {
                        if (!gif_increment_decode(gif, &frame_row, &frame_col, &index_counter)) {
                            goto gif_decode_end;
                        }
                    }

                    prev_code = code;
                    has_read_init_code = 1;
                }

                code = 0;
                code_bit_index = 0;
            }
        }
    }

gif_decode_end:
    gif_free_code_table(&code_table);
}

int gif_load_frame(struct gif *gif, struct gce *gce, uint8_t *buffer, FILE *file) { 
    /* Load frame with (optional) GCE data
       Buffer must be able to contain at least 9 bytes */

    struct id id;

    uint8_t *frame_codes = 0;
    uint32_t code_bytes = 0;
    uint8_t min_code_size;

    struct frame new_frame;
    struct frame *frame = (struct frame *) dyn_arr_append(&gif->frames, &new_frame);

    /* Graphic control extension */
    if (gce) {
        frame->delay = gce->delay;
        frame->disposal = gce->disposal;
        frame->has_transparency = gce->has_transparency;
        frame->transparent_index = gce->transparent_index;
    }
    else {
        frame->delay = 3;
        frame->disposal = DISPOSAL_NONE;
        frame->has_transparency = 0;
        frame->transparent_index = gif->bg_index;
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
    frame->has_lct = (buffer[8] >> 7) & 1U;

    #if DEBUG
        printf("Image descriptor:\n");
        printf("    Image left: %d\n", id.img_left);
        printf("    Image top: %d\n", id.img_top);
        printf("    Image width: %d\n", id.img_w);
        printf("    Image height: %d\n", id.img_h);
        printf("    LCT flag: %d\n", frame->has_lct);
        printf("    Interlace flag: %d\n", id.is_interlaced);
        printf("    Sort flag: %d\n", (buffer[8] >> 5) & 1U);
        printf("    LCT size: %d\n", buffer[8] & 7U);
    #endif

    /* Local color table */
    if (frame->has_lct) {
        gif_load_ct(gif, (1U << ((buffer[8] & 7U) + 1)) - 1, frame, file);
    }
    else { 
        if (!gif->has_gct) {
            printf("Error: Frame has no global or local color table\n");
            return ERROR_OUT;
        }
        frame->max_ct_color = gif->max_gct_color;
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
        frame_codes = (uint8_t *) realloc(frame_codes, code_bytes);
        fread(frame_codes + code_bytes - buffer[0], 1, buffer[0], file);
    }

    #if DEBUG
        printf("Decoding frame %ld (min code size: %d)...\n", gif->frames.length, min_code_size);
    #endif
    
    gif_decode(gif, &id, frame, frame_codes, code_bytes, min_code_size);
    free(frame_codes);

    #if DEBUG
        printf("Finished loading frame %ld\n", gif->frames.length);
    #endif

    return SUCC_OUT;
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
        printf("    Color resolution: %d\n", (buffer[4] >> 4) & 7U);
        printf("    Sort flag: %d\n", (buffer[4] >> 3) & 1U);
        printf("    GCT size: %d\n", buffer[4] & 7U);
        printf("    Background color index: %d\n", gif->bg_index);
        printf("    Pixel aspect ratio: %d\n", buffer[6]);
    #endif

    if (gif->has_gct) {
        gif_load_ct(gif, (1U << ((buffer[4] & 7U) + 1)) - 1, 0, file);
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
                gce.disposal = (buffer[0] >> 2) & 7U;
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

    dyn_arr_squeeze(&gif->frames);

    #if DEBUG
        printf("Loaded %ld frames\n", gif->frames.length);
    #endif

    fclose(file);

    return SUCC_OUT;
}

void gif_free(struct gif *gif) {
    size_t i;
    struct frame *frame;

    for (i = 0; i < gif->frames.length; ++i) {
        frame = (struct frame *) dyn_arr_get(&gif->frames, i);
        free(frame->ct_indices);
    }

    dyn_arr_free(&gif->frames);
}

int main() {
    struct gif gif;
    gif_init(&gif);
    if (gif_load(&gif, "img/sample6.gif") == ERROR_OUT) {
        return ERROR_OUT;
    }
    gif_free(&gif);
    return SUCC_OUT;
}

