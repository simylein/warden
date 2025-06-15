#pragma once

#include <sqlite3.h>
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

uint16_t reading_insert(sqlite3 *database, reading_t *reading);
