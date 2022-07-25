#include "dyn_arr.h"

void dyn_arr_init(struct dyn_arr *dyn_arr, uint16_t block_size, size_t elem_size) {
    dyn_arr->data = 0;
    dyn_arr->length = 0;
    dyn_arr->capacity = 0;
    dyn_arr->block_size = block_size;
    dyn_arr->elem_size = elem_size;
}

void *dyn_arr_append(struct dyn_arr *dyn_arr, void *elem) {
    /* Append elem to data, allocating more memory if needed */
    if (dyn_arr->length == dyn_arr->capacity) {
        dyn_arr->data = realloc(
            dyn_arr->data,
            (dyn_arr->capacity + dyn_arr->block_size) * dyn_arr->elem_size
        );
        dyn_arr->capacity += dyn_arr->block_size;
    }

    memcpy(
        dyn_arr->data + dyn_arr->length * dyn_arr->elem_size,
        elem,
        dyn_arr->elem_size
    );

    return dyn_arr->data + (dyn_arr->length++) * dyn_arr->elem_size;
}

void *dyn_arr_get(struct dyn_arr *dyn_arr, size_t index) {
    return dyn_arr->data + index * dyn_arr->elem_size;
}

void dyn_arr_squeeze(struct dyn_arr *dyn_arr) {
    /* Deallocate unused memory */
    dyn_arr->data = realloc(
        dyn_arr->data,
        dyn_arr->length * dyn_arr->elem_size
    );
}

void dyn_arr_free(struct dyn_arr *dyn_arr) {
    free(dyn_arr->data);
    dyn_arr->data = 0;
    dyn_arr->length = 0;
    dyn_arr->capacity = 0;
}

