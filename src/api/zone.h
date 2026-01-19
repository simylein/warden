#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
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
	const char *order;
	size_t order_len;
	const char *sort;
	size_t sort_len;
	uint8_t limit;
	uint32_t offset;
} zone_query_t;

typedef struct zone_row_t {
	uint8_t id;
	uint8_t name_len;
	uint8_t name;
	uint8_t color;
	uint8_t created_at;
	uint8_t updated_at_null;
	uint8_t updated_at;
	uint8_t reading_null;
	uint8_t reading_temperature;
	uint8_t reading_humidity;
	uint8_t reading_captured_at;
	uint8_t metric_null;
	uint8_t metric_photovoltaic;
	uint8_t metric_battery;
	uint8_t metric_captured_at;
	uint8_t buffer_null;
	uint8_t buffer_delay;
	uint8_t buffer_level;
	uint8_t buffer_captured_at;
	uint8_t size;
} zone_row_t;

extern const char *zone_file;

extern const zone_row_t zone_row;

extern const char *zone_table;
extern const char *zone_schema;

uint16_t zone_existing(octet_t *db, zone_t *zone);
uint16_t zone_lookup(octet_t *db, zone_t *zone);

uint16_t zone_select(octet_t *db, bwt_t *bwt, zone_query_t *query, response_t *response, uint8_t *zones_len);
uint16_t zone_select_one(octet_t *db, bwt_t *bwt, zone_t *zone, response_t *response);
uint16_t zone_insert(octet_t *db, zone_t *zone);
uint16_t zone_update(octet_t *db, zone_t *zone);

void zone_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void zone_find_one(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void zone_modify(octet_t *db, request_t *request, response_t *response);
