#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include "zone.h"
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

typedef struct buffer_row_t {
	uint8_t id;
	uint8_t delay;
	uint8_t level;
	uint8_t captured_at;
	uint8_t uplink_id;
	uint8_t size;
} buffer_row_t;

extern const char *buffer_file;

extern const buffer_row_t buffer_row;

extern const char *buffer_table;
extern const char *buffer_schema;

uint16_t buffer_select(octet_t *db, bwt_t *bwt, buffer_query_t *query, response_t *response, uint16_t *buffers_len);
uint16_t buffer_select_by_device(octet_t *db, device_t *device, buffer_query_t *query, response_t *response,
																 uint16_t *buffers_len);
uint16_t buffer_select_by_zone(sqlite3 *database, bwt_t *bwt, zone_t *zone, buffer_query_t *query, response_t *response,
															 uint16_t *buffers_len);
uint16_t buffer_insert(octet_t *db, buffer_t *buffer);

void buffer_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void buffer_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void buffer_find_by_zone(octet_t *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
