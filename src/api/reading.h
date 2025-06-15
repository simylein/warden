#pragma once

#include <stdint.h>
#include <time.h>

typedef struct reading_t {
	uint8_t (*id)[16];
	float temperature;
	float humidity;
	time_t captured_at;
} reading_t;

extern const char *reading_table;
extern const char *reading_schema;
