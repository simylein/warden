#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <stdint.h>
#include <time.h>

typedef struct user_t {
	uint8_t (*id)[16];
	char *username;
	uint8_t username_len;
	char *password;
	uint8_t password_len;
	time_t *signup_at;
	time_t *signin_at;
	uint8_t (*permissions)[8];
} user_t;

typedef struct user_query_t {
	const char *order;
	size_t order_len;
	const char *sort;
	size_t sort_len;
	uint8_t limit;
	uint32_t offset;
} user_query_t;

typedef struct user_row_t {
	uint8_t id;
	uint8_t username_len;
	uint8_t username;
	uint8_t password;
	uint8_t signup_at;
	uint8_t signin_at;
	uint8_t permissions;
	uint8_t size;
} user_row_t;

extern const char *user_file;

extern const user_row_t user_row;

uint16_t user_existing(octet_t *db, user_t *user);

uint16_t user_select(octet_t *db, user_query_t *query, response_t *response, uint8_t *users_len);
uint16_t user_select_one(octet_t *db, user_t *user, response_t *response);
uint16_t user_insert(octet_t *db, user_t *user);
uint16_t user_update(octet_t *db, user_t *user);
uint16_t user_delete(octet_t *db, user_t *user);

void user_find(octet_t *db, request_t *request, response_t *response);
void user_find_one(octet_t *db, request_t *request, response_t *response);
void user_profile(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void user_signup(octet_t *db, request_t *request, response_t *response);
void user_signin(octet_t *db, request_t *request, response_t *response);
void user_remove(octet_t *db, request_t *request, response_t *response);
