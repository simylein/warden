#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct reading_t {
	uint8_t (*id)[16];
	float temperature;
	float humidity;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} reading_t;

typedef struct reading_query_t {
	time_t from;
	time_t to;
} reading_query_t;

extern const char *reading_table;
extern const char *reading_schema;

uint16_t reading_insert(sqlite3 *database, reading_t *reading);

void reading_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
