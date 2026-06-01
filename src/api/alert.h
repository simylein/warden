#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <stdint.h>
#include <time.h>

typedef struct alert_t {
	uint8_t severity;
	uint8_t field;
	int32_t value;
	time_t issued_at;
	time_t *resolved_at;
	uint8_t (*device_id)[8];
} alert_t;

typedef struct alert_query_t {
	uint8_t limit;
	uint32_t offset;
} alert_query_t;

typedef struct alert_row_t {
	uint8_t severity;
	uint8_t field;
	uint8_t value;
	uint8_t issued_at;
	uint8_t resolved_at_null;
	uint8_t resolved_at;
	uint8_t size;
} alert_row_t;

extern const char *alert_file;

extern const alert_row_t alert_row;

uint16_t alert_select(octet_t *db, bwt_t *bwt, alert_query_t *query, response_t *response, uint8_t *alerts_len);
uint16_t alert_insert(octet_t *db, alert_t *alert);

void alert_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
