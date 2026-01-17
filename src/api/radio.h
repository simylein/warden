#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct radio_t {
	uint8_t (*id)[16];
	uint32_t frequency;
	uint32_t bandwidth;
	uint8_t coding_rate;
	uint8_t spreading_factor;
	uint8_t preamble_length;
	uint8_t tx_power;
	uint8_t sync_word;
	bool checksum;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} radio_t;

extern const char *radio_table;
extern const char *radio_schema;

uint16_t radio_select_one_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response);
uint16_t radio_insert(sqlite3 *database, radio_t *radio);

void radio_find_one_by_device(octet_t *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
