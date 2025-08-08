#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
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

extern const char *metric_table;
extern const char *metric_schema;

uint16_t metric_insert(sqlite3 *database, metric_t *metric);

void metric_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
