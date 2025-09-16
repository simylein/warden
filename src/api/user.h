#pragma once

#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
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
	uint8_t (*permissions)[4];
} user_t;

typedef struct user_query_t {
	uint8_t limit;
	uint32_t offset;
} user_query_t;

extern const char *user_table;
extern const char *user_schema;

uint16_t user_existing(sqlite3 *database, user_t *user);

uint16_t user_select(sqlite3 *database, user_query_t *query, response_t *response, uint8_t *users_len);
uint16_t user_select_one(sqlite3 *database, user_t *user, response_t *response);
uint16_t user_insert(sqlite3 *database, user_t *user);
uint16_t user_update(sqlite3 *database, user_t *user);
uint16_t user_delete(sqlite3 *database, user_t *user);

void user_find(sqlite3 *database, request_t *request, response_t *response);
void user_find_one(sqlite3 *database, request_t *request, response_t *response);
void user_signup(sqlite3 *database, request_t *request, response_t *response);
void user_signin(sqlite3 *database, request_t *request, response_t *response);
void user_remove(sqlite3 *database, request_t *request, response_t *response);
