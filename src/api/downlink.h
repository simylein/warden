#pragma once

#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct downlink_t {
	uint8_t (*id)[16];
	uint8_t kind;
	uint8_t *data;
	uint8_t data_len;
	uint16_t airtime;
	uint32_t frequency;
	uint32_t bandwidth;
	uint8_t tx_power;
	uint8_t sf;
	time_t sent_at;
	uint8_t (*device_id)[16];
} downlink_t;

extern const char *downlink_table;
extern const char *downlink_schema;

void downlink_create(sqlite3 *database, request_t *request, response_t *response);
