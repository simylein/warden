#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "user.h"
#include <stdint.h>
#include <time.h>

typedef struct zone_t {
	uint8_t (*id)[8];
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
	uint16_t id;
	uint16_t name_len;
	uint16_t name;
	uint16_t color;
	uint16_t created_at;
	uint16_t updated_at_null;
	uint16_t updated_at;
	uint16_t reading_null;
	uint16_t reading_temperature;
	uint16_t reading_humidity;
	uint16_t reading_dewpoint;
	uint16_t reading_captured_at;
	uint16_t metric_null;
	uint16_t metric_photovoltaic;
	uint16_t metric_battery;
	uint16_t metric_captured_at;
	uint16_t buffer_null;
	uint16_t buffer_delay;
	uint16_t buffer_level;
	uint16_t buffer_captured_at;
	uint16_t size;
} zone_row_t;

extern const char *zone_file;

extern const zone_row_t zone_row;

uint16_t zone_existing(octet_t *db, zone_t *zone);
uint16_t zone_lookup(octet_t *db, zone_t *zone);

uint16_t zone_select(octet_t *db, bwt_t *bwt, zone_query_t *query, response_t *response, uint8_t *zones_len);
uint16_t zone_select_one(octet_t *db, bwt_t *bwt, zone_t *zone, response_t *response);
uint16_t zone_select_by_user(octet_t *db, user_t *user, zone_query_t *query, response_t *response, uint8_t *zones_len);
uint16_t zone_insert(octet_t *db, zone_t *zone);
uint16_t zone_update(octet_t *db, zone_t *zone);
uint16_t zone_update_latest(octet_t *db, zone_t *zone);

void zone_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void zone_find_one(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void zone_find_by_user(octet_t *db, request_t *request, response_t *response);
void zone_modify(octet_t *db, request_t *request, response_t *response);
