#pragma once

#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "user.h"
#include <sqlite3.h>
#include <stdint.h>

typedef struct user_device_t {
	uint8_t (*user_id)[16];
	uint8_t (*device_id)[16];
} user_device_t;

typedef struct user_device_row_t {
	uint8_t user_id;
	uint8_t device_id;
	uint8_t size;
} user_device_row_t;

extern const user_device_row_t user_device_row;

extern const char *user_device_table;
extern const char *user_device_schema;

uint16_t user_device_existing(octet_t *db, user_device_t *user_device);
uint16_t user_device_select_by_user(octet_t *db, user_t *user, uint8_t *user_devices_len);
uint16_t user_device_insert(octet_t *db, user_device_t *user_device);
uint16_t user_device_delete(sqlite3 *database, user_device_t *user_device);

void user_device_create(octet_t *db, request_t *request, response_t *response);
void user_device_remove(octet_t *db, sqlite3 *database, request_t *request, response_t *response);
