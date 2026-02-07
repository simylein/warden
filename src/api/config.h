#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct config_t {
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

typedef struct config_row_t {
	uint8_t led_debug;
	uint8_t reading_enable;
	uint8_t metric_enable;
	uint8_t buffer_enable;
	uint8_t reading_interval;
	uint8_t metric_interval;
	uint8_t buffer_interval;
	uint8_t captured_at;
	uint8_t uplink_id;
	uint8_t size;
} config_row_t;

extern const char *config_file;

extern const config_row_t config_row;

uint16_t config_select_one_by_device(octet_t *db, device_t *device, response_t *response);
uint16_t config_insert(octet_t *db, config_t *config);

void config_find_one_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void config_modify(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
