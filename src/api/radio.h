#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct radio_t {
	uint32_t frequency;
	uint32_t bandwidth;
	uint8_t coding_rate;
	uint8_t spreading_factor;
	uint8_t preamble_length;
	uint8_t tx_power;
	uint8_t sync_word;
	bool checksum;
	time_t captured_at;
	uint8_t (*device_id)[16];
} radio_t;

typedef struct radio_row_t {
	uint8_t frequency;
	uint8_t bandwidth;
	uint8_t coding_rate;
	uint8_t spreading_factor;
	uint8_t preamble_length;
	uint8_t tx_power;
	uint8_t sync_word;
	uint8_t checksum;
	uint8_t captured_at;
	uint8_t size;
} radio_row_t;

extern const char *radio_file;

extern const radio_row_t radio_row;

uint16_t radio_select_one_by_device(octet_t *db, device_t *device, response_t *response);
uint16_t radio_insert(octet_t *db, radio_t *radio);

void radio_find_one_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void radio_modify(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
