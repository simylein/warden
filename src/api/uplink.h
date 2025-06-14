#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>

typedef struct uplink_t {
	uint8_t (*id)[16];
	uint8_t kind;
	uint8_t *data;
	uint8_t data_len;
	uint16_t airtime;
	uint32_t frequency;
	uint32_t bandwidth;
	int16_t rssi;
	int8_t snr;
	uint8_t sf;
	time_t received_at;
	uint8_t (*device_id)[16];
} uplink_t;

typedef struct uplink_query_t {
	uint8_t limit;
	uint32_t offset;
} uplink_query_t;

extern const char *uplink_table;
extern const char *uplink_schema;

uint16_t uplink_insert(sqlite3 *database, uplink_t *uplink);

void uplink_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
