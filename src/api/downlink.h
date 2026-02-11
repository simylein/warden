#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <stdint.h>
#include <time.h>

typedef struct downlink_t {
	uint16_t frame;
	uint8_t kind;
	uint8_t *data;
	uint8_t data_len;
	uint16_t airtime;
	uint32_t frequency;
	uint32_t bandwidth;
	uint8_t sf;
	uint8_t cr;
	uint8_t tx_power;
	uint8_t preamble_len;
	time_t sent_at;
	uint8_t (*device_id)[8];
} downlink_t;

typedef struct downlink_query_t {
	uint8_t limit;
	uint32_t offset;
} downlink_query_t;

typedef struct downlink_row_t {
	uint8_t frame;
	uint8_t kind;
	uint8_t data_len;
	uint8_t data;
	uint8_t airtime;
	uint8_t frequency;
	uint8_t bandwidth;
	uint8_t sf;
	uint8_t cr;
	uint8_t tx_power;
	uint8_t preamble_len;
	uint8_t sent_at;
	uint8_t size;
} downlink_row_t;

extern const char *downlink_file;

extern const downlink_row_t downlink_row;

uint16_t downlink_select(octet_t *db, bwt_t *bwt, downlink_query_t *query, response_t *response, uint8_t *downlinks_len);
uint16_t downlink_select_by_device(octet_t *db, device_t *device, downlink_query_t *query, response_t *response,
																	 uint8_t *downlinks_len);
uint16_t downlink_insert(octet_t *db, downlink_t *downlink);

void downlink_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void downlink_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void downlink_create(octet_t *db, request_t *request, response_t *response);
