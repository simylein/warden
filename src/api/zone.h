#pragma once

#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct zone_t {
	uint8_t (*id)[16];
	char *name;
	uint8_t name_len;
	uint8_t (*color)[12];
	time_t *created_at;
	time_t *updated_at;
} zone_t;

typedef struct zone_query_t {
	char *order;
	uint8_t order_len;
	char *sort;
	uint8_t sort_len;
} zone_query_t;

extern const char *zone_table;
extern const char *zone_schema;

uint16_t zone_insert(sqlite3 *database, zone_t *zone);
