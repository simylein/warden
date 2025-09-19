#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct buffer_t {
	uint8_t (*id)[16];
	uint32_t delay;
	uint16_t level;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} buffer_t;

typedef struct buffer_query_t {
	time_t from;
	time_t to;
	uint16_t bucket;
} buffer_query_t;

extern const char *buffer_table;
extern const char *buffer_schema;

uint16_t buffer_select_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, buffer_query_t *query, response_t *response,
																 uint16_t *buffers_len);
uint16_t buffer_insert(sqlite3 *database, buffer_t *buffer);

void buffer_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
