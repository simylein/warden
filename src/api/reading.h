#pragma once

#include <stdint.h>

typedef struct reading_t {
	uint8_t (*id)[16];
} reading_t;

extern const char *reading_table;
extern const char *reading_schema;
