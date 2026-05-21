#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <stdint.h>
#include <time.h>

typedef struct rule_t {
	uint8_t severity;
	uint8_t field;
	int32_t activate;
	int32_t disable;
	time_t created_at;
	time_t *updated_at;
	uint8_t (*device_id)[8];
} rule_t;

typedef struct rule_query_t {
	uint8_t limit;
	uint32_t offset;
} rule_query_t;

typedef struct rule_row_t {
	uint8_t severity;
	uint8_t field;
	uint8_t activate;
	uint8_t disable;
	uint8_t created_at;
	uint8_t updated_at_null;
	uint8_t updated_at;
	uint8_t size;
} rule_row_t;

extern const char *rule_file;

extern const rule_row_t rule_tow;

uint16_t rule_select_by_device(octet_t *db, device_t *device, rule_query_t *query, response_t *response, uint8_t *rules_len);

void rule_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
