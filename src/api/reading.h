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
	uint16_t bucket;
} reading_query_t;

typedef struct reading_row_t {
	uint8_t id;
	uint8_t temperature;
	uint8_t humidity;
	uint8_t captured_at;
	uint8_t uplink_id;
	uint8_t size;
} reading_row_t;

extern const char *reading_file;

extern const reading_row_t reading_row;

extern const char *reading_table;
extern const char *reading_schema;

uint16_t reading_select(sqlite3 *database, bwt_t *bwt, reading_query_t *query, response_t *response, uint16_t *readings_len);
uint16_t reading_select_by_device(octet_t *db, device_t *device, reading_query_t *query, response_t *response,
																	uint16_t *readings_len);
uint16_t reading_select_by_zone(sqlite3 *database, bwt_t *bwt, zone_t *zone, reading_query_t *query, response_t *response,
																uint16_t *readings_len);
uint16_t reading_insert(octet_t *db, reading_t *reading);

void reading_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void reading_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void reading_find_by_zone(octet_t *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
