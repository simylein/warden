#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include "zone.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct metric_t {
	uint8_t (*id)[16];
	float photovoltaic;
	float battery;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} metric_t;

typedef struct metric_query_t {
	time_t from;
	time_t to;
	uint16_t bucket;
} metric_query_t;

typedef struct metric_row_t {
	uint8_t id;
	uint8_t photovoltaic;
	uint8_t battery;
	uint8_t captured_at;
	uint8_t uplink_id;
	uint8_t size;
} metric_row_t;

extern const metric_row_t metric_row;

extern const char *metric_table;
extern const char *metric_schema;

uint16_t metric_select(sqlite3 *database, bwt_t *bwt, metric_query_t *query, response_t *response, uint16_t *metrics_len);
uint16_t metric_select_by_device(const char *db, device_t *device, metric_query_t *query, response_t *response,
																 uint16_t *metrics_len);
uint16_t metric_select_by_zone(sqlite3 *database, bwt_t *bwt, zone_t *zone, metric_query_t *query, response_t *response,
															 uint16_t *metrics_len);
uint16_t metric_insert(const char *db, metric_t *metric);

void metric_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void metric_find_by_device(const char *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void metric_find_by_zone(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
