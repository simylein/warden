#pragma once

#include "../lib/octet.h"
#include <stdint.h>

typedef struct email_t {
	char *address;
	uint8_t address_len;
	uint16_t port;
	char *from;
	uint8_t from_len;
	char *to;
	uint8_t to_len;
} email_t;

typedef struct email_row_t {
	uint8_t address_len;
	uint8_t address;
	uint8_t port;
	uint8_t from_len;
	uint8_t from;
	uint8_t to_len;
	uint8_t to;
	uint8_t size;
} email_row_t;

extern const char *email_file;

extern const email_row_t email_row;

uint16_t email_select_one(octet_t *db, email_t *email);
uint16_t email_insert(octet_t *db, email_t *email);
