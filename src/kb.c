#include "kb.h"

int kb_init() {
    /* Open keyboard device (must be run as root) */
    if (!(kb_file = fopen("/dev/input/by-path/platform-i8042-serio-0-event-kbd", "r"))) {
        perror("Error [fopen kb]");
        return ERROR_OUT;
    }
    return SUCC_OUT;
}

void kb_update_map() {
    /* Zero out kb_map and fill with updated values */
    memset(kb_map, 0, sizeof(kb_map));
    ioctl(fileno(kb_file), EVIOCGKEY(sizeof(kb_map)), kb_map);
}

uint8_t kb_read_map(int key_code) {
    return (kb_map[key_code / 8] & (1U << (key_code % 8))) > 0;
}

void kb_free() {
    fclose(kb_file);
}

