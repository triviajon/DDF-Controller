#ifndef KB_H
#define KB_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <glob.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "global_defines.h"

static FILE *kb_file;
static char kb_map[KEY_MAX / 8 + 1];

int kb_init();
void kb_update_map();
uint8_t kb_read_map(int key_code);
void kb_free();

#endif

