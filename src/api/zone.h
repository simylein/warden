#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
#include <stdint.h>
#include <time.h>

typedef struct zone_t {
	uint8_t (*id)[16];
	char *name;
	uint8_t name_len;
	uint8_t (*color)[12];
	time_t *created_at;
	time_t *updated_at;
} zone_t;

typedef struct zone_query_t {
	char *order;
	uint8_t order_len;
	char *sort;
	uint8_t sort_len;
} zone_query_t;

extern const char *zone_table;
extern const char *zone_schema;

uint16_t zone_existing(sqlite3 *database, bwt_t *bwt, zone_t *zone);

uint16_t zone_select(sqlite3 *database, bwt_t *bwt, zone_query_t *query, response_t *response, uint8_t *zones_len);
uint16_t zone_select_one(sqlite3 *database, bwt_t *bwt, zone_t *zone, response_t *response);
uint16_t zone_insert(sqlite3 *database, zone_t *zone);
uint16_t zone_update(sqlite3 *database, zone_t *zone);

void zone_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void zone_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void zone_modify(sqlite3 *database, request_t *request, response_t *response);
