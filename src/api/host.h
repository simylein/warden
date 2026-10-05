#pragma once

#include "../lib/octet.h"
#include <stdint.h>

typedef struct host_t {
	uint8_t (*id)[8];
	char *address;
	uint8_t address_len;
	uint16_t port;
	char *username;
	uint8_t username_len;
	char *password;
	uint8_t password_len;
} host_t;

typedef struct host_row_t {
	uint16_t id;
	uint16_t address_len;
	uint16_t address;
	uint16_t port;
	uint16_t username_len;
	uint16_t username;
	uint16_t password_len;
	uint16_t password;
	uint16_t size;
} host_row_t;

extern const char *host_file;

extern const host_row_t host_row;

uint16_t host_select_one(octet_t *db, host_t *host);
uint16_t host_insert(octet_t *db, host_t *host);
