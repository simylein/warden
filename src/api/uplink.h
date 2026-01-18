#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include "zone.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct uplink_t {
	uint8_t (*id)[16];
	uint16_t frame;
	uint8_t kind;
	uint8_t *data;
	uint8_t data_len;
	uint16_t airtime;
	uint32_t frequency;
	uint32_t bandwidth;
	int16_t rssi;
	int8_t snr;
	uint8_t sf;
	uint8_t cr;
	uint8_t tx_power;
	uint8_t preamble_len;
	time_t received_at;
	uint8_t (*device_id)[16];
} uplink_t;

typedef struct uplink_query_t {
	uint8_t limit;
	uint32_t offset;
} uplink_query_t;

typedef struct uplink_signal_query_t {
	time_t from;
	time_t to;
	uint16_t bucket;
} uplink_signal_query_t;

typedef struct uplink_row_t {
	uint8_t id;
	uint8_t frame;
	uint8_t kind;
	uint8_t data_len;
	uint8_t data;
	uint8_t airtime;
	uint8_t frequency;
	uint8_t bandwidth;
	uint8_t rssi;
	uint8_t snr;
	uint8_t sf;
	uint8_t cr;
	uint8_t tx_power;
	uint8_t preamble_len;
	uint8_t received_at;
	uint8_t size;
} uplink_row_t;

extern const uplink_row_t uplink_row;

extern const char *uplink_table;
extern const char *uplink_schema;

uint16_t uplink_existing(sqlite3 *database, bwt_t *bwt, uplink_t *uplink);

uint16_t uplink_select(sqlite3 *database, bwt_t *bwt, uplink_query_t *query, response_t *response, uint8_t *uplinks_len);
uint16_t uplink_select_one(sqlite3 *database, bwt_t *bwt, uplink_t *uplink, response_t *response);
uint16_t uplink_select_by_device(octet_t *db, device_t *device, uplink_query_t *query, response_t *response,
																 uint8_t *uplinks_len);
uint16_t uplink_signal_select_by_device(octet_t *db, device_t *device, uplink_signal_query_t *query, response_t *response,
																				uint16_t *signals_len);
uint16_t uplink_signal_select_by_zone(sqlite3 *database, bwt_t *bwt, zone_t *zone, uplink_signal_query_t *query,
																			response_t *response, uint16_t *signals_len);
uint16_t uplink_insert(octet_t *db, uplink_t *uplink);

void uplink_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void uplink_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void uplink_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void uplink_signal_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void uplink_signal_find_by_zone(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void uplink_create(octet_t *db, request_t *request, response_t *response);
