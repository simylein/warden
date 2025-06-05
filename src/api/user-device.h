#pragma once

#include <sqlite3.h>
#include <stdint.h>

typedef struct user_device_t {
	uint8_t (*user_id)[16];
	uint8_t (*device_id)[16];
} user_device_t;

extern const char *user_device_table;
extern const char *user_device_schema;

uint16_t user_device_insert(sqlite3 *database, user_device_t *user_device);
