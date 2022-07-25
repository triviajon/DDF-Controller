#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

struct dyn_arr {
    uint8_t *data;

    /* Unit: element counts */
    size_t length;
    size_t capacity;
    uint16_t block_size;

    /* Unit: bytes */
    size_t elem_size;
};

void dyn_arr_init(struct dyn_arr *dyn_arr, uint16_t block_size, size_t elem_size);
void *dyn_arr_append(struct dyn_arr *dyn_arr, void *elem);
void *dyn_arr_get(struct dyn_arr *dyn_arr, size_t index);
void dyn_arr_squeeze(struct dyn_arr *dyn_arr);
void dyn_arr_free(struct dyn_arr *dyn_arr);

#endif

