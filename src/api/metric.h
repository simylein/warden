#pragma once

#include <stdint.h>

typedef struct metric_t {
	uint8_t (*id)[16];
} metric_t;

extern const char *metric_table;
extern const char *metric_schema;
