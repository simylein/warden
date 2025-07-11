#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct device_t {
	uint8_t (*id)[16];
	char *name;
	uint8_t name_len;
	char *type;
	uint8_t type_len;
	time_t *created_at;
	time_t *updated_at;
} device_t;

extern const char *device_table;
extern const char *device_schema;

uint16_t device_existing(sqlite3 *database, bwt_t *bwt, device_t *device);

uint16_t device_select(sqlite3 *database, bwt_t *bwt, response_t *response, uint8_t *devices_len);
uint16_t device_select_one(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response);
uint16_t device_insert(sqlite3 *database, device_t *device);

void device_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void device_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
