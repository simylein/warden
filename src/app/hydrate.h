#pragma once

#include "file.h"
#include <stdint.h>

typedef struct class_t {
	char *ptr;
	uint8_t len;
} class_t;

int extract(file_t *file, class_t (*classes)[128], uint8_t *classes_len);
