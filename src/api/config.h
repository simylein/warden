#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct config_t {
	uint8_t (*id)[16];
	bool led_debug;
	bool reading_enable;
	bool metric_enable;
	bool buffer_enable;
	uint16_t reading_interval;
	uint16_t metric_interval;
	uint16_t buffer_interval;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} config_t;

extern const char *config_table;
extern const char *config_schema;

uint16_t config_select_one_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response);
uint16_t config_insert(sqlite3 *database, config_t *config);

void config_find_one_by_device(octet_t *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
