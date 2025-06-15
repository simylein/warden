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
	time_t *created_at;
	time_t *updated_at;
} device_t;

extern const char *device_table;
extern const char *device_schema;

uint16_t device_insert(sqlite3 *database, device_t *device);

void device_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
