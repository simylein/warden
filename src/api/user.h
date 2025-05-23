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
} user_t;

extern const char *user_table;
extern const char *user_schema;

void user_signup(sqlite3 *database, request_t *request, response_t *response);
void user_signin(sqlite3 *database, request_t *request, response_t *response);
