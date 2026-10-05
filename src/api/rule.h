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
	uint8_t edge;
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
	uint16_t severity;
	uint16_t field;
	uint16_t edge;
	uint16_t activate;
	uint16_t disable;
	uint16_t created_at;
	uint16_t updated_at_null;
	uint16_t updated_at;
	uint16_t size;
} rule_row_t;

extern const char *rule_file;

extern const rule_row_t rule_row;

uint16_t rule_select_by_device(octet_t *db, device_t *device, rule_query_t *query, response_t *response, uint8_t *rules_len);
uint16_t rule_insert(octet_t *db, rule_t *rule);
uint16_t rule_update(octet_t *db, rule_t *rule);
uint16_t rule_delete(octet_t *db, rule_t *rule);

void rule_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void rule_create(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void rule_modify(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void rule_remove(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
