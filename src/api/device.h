#pragma once

#include "../lib/bwt.h"
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

extern const char *device_table;
extern const char *device_schema;

uint16_t device_existing(sqlite3 *database, bwt_t *bwt, device_t *device);

uint16_t device_select(sqlite3 *database, bwt_t *bwt, device_query_t *query, response_t *response, uint8_t *devices_len);
uint16_t device_select_one(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response);
uint16_t device_select_by_user(sqlite3 *database, user_t *user, device_query_t *query, response_t *response,
															 uint8_t *devices_len);
uint16_t device_insert(sqlite3 *database, device_t *device);
uint16_t device_update(sqlite3 *database, device_t *device);

void device_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void device_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void device_find_by_user(sqlite3 *database, request_t *request, response_t *response);
void device_modify(sqlite3 *database, request_t *request, response_t *response);
