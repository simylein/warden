#pragma once

#include "../lib/octet.h"
#include "user.h"
#include <stdint.h>

typedef struct user_zone_t {
	uint8_t (*user_id)[16];
	uint8_t (*zone_id)[16];
} user_zone_t;

typedef struct user_zone_row_t {
	uint8_t user_id;
	uint8_t zone_id;
	uint8_t size;
} user_zone_row_t;

extern const char *user_zone_file;

extern const user_zone_row_t user_zone_row;

uint16_t user_zone_existing(octet_t *db, user_zone_t *user_zone);
uint16_t user_zone_select_by_user(octet_t *db, user_t *user, uint8_t *user_zones_len);
