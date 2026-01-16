#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "user.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct device_t {
	uint8_t (*id)[16];
	char *name;
	uint8_t name_len;
	uint8_t (*zone_id)[16];
	char *zone_name;
	uint8_t zone_name_len;
	uint8_t (*zone_color)[12];
	char *firmware;
	uint8_t firmware_len;
	char *hardware;
	uint8_t hardware_len;
	time_t *created_at;
	time_t *updated_at;
} device_t;

typedef struct device_query_t {
	const char *order;
	size_t order_len;
	const char *sort;
	size_t sort_len;
	uint8_t limit;
	uint32_t offset;
} device_query_t;

typedef struct device_row_t {
	uint8_t id;
	uint8_t name_len;
	uint8_t name;
	uint8_t firmware_len;
	uint8_t firmware;
	uint8_t hardware_len;
	uint8_t hardware;
	uint8_t created_at;
	uint8_t updated_at_null;
	uint8_t updated_at;
	uint8_t zone_null;
	uint8_t zone_id;
	uint8_t zone_name_len;
	uint8_t zone_name;
	uint8_t zone_color;
	uint8_t reading_null;
	uint8_t reading_id;
	uint8_t reading_temperature;
	uint8_t reading_humidity;
	uint8_t reading_captured_at;
	uint8_t metric_null;
	uint8_t metric_id;
	uint8_t metric_photovoltaic;
	uint8_t metric_battery;
	uint8_t metric_captured_at;
	uint8_t buffer_null;
	uint8_t buffer_id;
	uint8_t buffer_delay;
	uint8_t buffer_level;
	uint8_t buffer_captured_at;
	uint8_t size;
} device_row_t;

extern const device_row_t device_row;

extern const char *device_table;
extern const char *device_schema;

uint16_t device_existing(sqlite3 *database, bwt_t *bwt, device_t *device);

uint16_t device_select(octet_t *db, bwt_t *bwt, device_query_t *query, response_t *response, uint8_t *devices_len);
uint16_t device_select_one(octet_t *db, bwt_t *bwt, device_t *device, response_t *response);
uint16_t device_select_by_user(sqlite3 *database, user_t *user, device_query_t *query, response_t *response,
															 uint8_t *devices_len);
uint16_t device_insert(octet_t *db, device_t *device);
uint16_t device_update(sqlite3 *database, device_t *device);

void device_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void device_find_one(octet_t *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void device_find_by_user(octet_t *db, sqlite3 *database, request_t *request, response_t *response);
void device_modify(sqlite3 *database, request_t *request, response_t *response);
