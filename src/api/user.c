#include "user.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/config.h"
#include "../lib/endian.h"
#include "../lib/format.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/sha256.h"
#include "database.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *user_table = "user";
const char *user_schema = "create table user ("
													"id blob primary key, "
													"username text not null unique, "
													"password blob not null, "
													"signup_at timestamp not null, "
													"signin_at timestamp not null, "
													"permissions blob not null"
													")";

uint16_t user_existing(sqlite3 *database, user_t *user) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select user.id from user where user.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user->id, sizeof(*user->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*user->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*user->id));
			status = 500;
			goto cleanup;
		}
		memcpy(user->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("user %02x%02x not found\n", (*user->id)[0], (*user->id)[1]);
		status = 404;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t user_select(sqlite3 *database, user_query_t *query, response_t *response, uint8_t *users_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, username, signup_at, signin_at, permissions from user "
										"order by "
										"case when ?1 = 'id' and ?2 = 'asc' then user.id end asc, "
										"case when ?1 = 'id' and ?2 = 'desc' then user.id end desc, "
										"case when ?1 = 'username' and ?2 = 'asc' then user.username end asc, "
										"case when ?1 = 'username' and ?2 = 'desc' then user.username end desc, "
										"case when ?1 = 'signupAt' and ?2 = 'asc' then user.signup_at end asc, "
										"case when ?1 = 'signupAt' and ?2 = 'desc' then user.signup_at end desc, "
										"case when ?1 = 'signinAt' and ?2 = 'asc' then user.signin_at end asc, "
										"case when ?1 = 'signinAt' and ?2 = 'desc' then user.signin_at end desc "
										"limit ?3 offset ?4";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, query->order, (uint8_t)query->order_len, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, query->sort, (uint8_t)query->sort_len, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, query->limit);
	sqlite3_bind_int64(stmt, 4, query->offset);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const uint8_t *id = sqlite3_column_blob(stmt, 0);
			const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
			if (id_len != sizeof(*((user_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((user_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *username = sqlite3_column_text(stmt, 1);
			const size_t username_len = (size_t)sqlite3_column_bytes(stmt, 1);
			const time_t signup_at = (time_t)sqlite3_column_int64(stmt, 2);
			const time_t signin_at = (time_t)sqlite3_column_int64(stmt, 3);
			const uint8_t *permissions = sqlite3_column_blob(stmt, 4);
			const size_t permissions_len = (size_t)sqlite3_column_bytes(stmt, 4);
			if (permissions_len > UINT8_MAX) {
				error("permissions length %zu exceeds buffer length %hhu\n", permissions_len, UINT8_MAX);
				status = 500;
				goto cleanup;
			}
			body_write(response, id, id_len);
			body_write(response, username, username_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint64_t[]){hton64((uint64_t)signup_at)}, sizeof(signup_at));
			body_write(response, (uint64_t[]){hton64((uint64_t)signin_at)}, sizeof(signin_at));
			body_write(response, &permissions_len, sizeof(uint8_t));
			body_write(response, permissions, permissions_len);
			*users_len += 1;
		} else if (result == SQLITE_DONE) {
			status = 0;
			break;
		} else {
			status = database_error(database, result);
			goto cleanup;
		}
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t user_select_one(sqlite3 *database, user_t *user, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, username, signup_at, signin_at, permissions from user "
										"where id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user->id, sizeof(*user->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*((user_t *)0)->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((user_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t *username = sqlite3_column_text(stmt, 1);
		const size_t username_len = (size_t)sqlite3_column_bytes(stmt, 1);
		const time_t signup_at = (time_t)sqlite3_column_int64(stmt, 2);
		const time_t signin_at = (time_t)sqlite3_column_int64(stmt, 3);
		const uint8_t *permissions = sqlite3_column_blob(stmt, 4);
		const size_t permissions_len = (size_t)sqlite3_column_bytes(stmt, 4);
		if (permissions_len > UINT8_MAX) {
			error("permissions length %zu exceeds buffer length %hhu\n", permissions_len, UINT8_MAX);
			status = 500;
			goto cleanup;
		}
		body_write(response, id, id_len);
		body_write(response, username, username_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, (uint64_t[]){hton64((uint64_t)signup_at)}, sizeof(signup_at));
		body_write(response, (uint64_t[]){hton64((uint64_t)signin_at)}, sizeof(signin_at));
		body_write(response, &permissions_len, sizeof(uint8_t));
		body_write(response, permissions, permissions_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("user %02x%02x not found\n", (*user->id)[0], (*user->id)[1]);
		status = 404;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

int user_parse(user_t *user, request_t *request) {
	request->body.pos = 0;

	uint8_t stage = 0;

	user->username_len = 0;
	const uint8_t username_index = (uint8_t)request->body.pos;
	while (stage == 0 && user->username_len < 16 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 1;
		} else {
			user->username_len++;
		}
	}
	user->username = &request->body.ptr[username_index];
	if (stage != 1) {
		debug("found username with %hhu bytes\n", user->username_len);
		return -1;
	}

	user->password_len = 0;
	const uint8_t password_index = (uint8_t)request->body.pos;
	while (stage == 1 && user->password_len < 64 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 2;
		} else {
			user->password_len++;
		}
	}
	user->password = &request->body.ptr[password_index];
	if (stage != 2) {
		debug("found password with %hhu bytes\n", user->password_len);
		return -1;
	}

	trace("username %hhu bytes and password %hhu bytes\n", user->username_len, user->password_len);
	return 0;
}

int user_validate(user_t *user) {
	if (user->username_len < 4) {
		return -1;
	}

	uint8_t username_index = 0;
	while (username_index < user->username_len) {
		char *byte = &user->username[username_index];
		if (*byte < 'a' || *byte > 'z') {
			debug("username contains invalid character %02x\n", *byte);
			return -1;
		}
		username_index++;
	}

	if (user->password_len < 4) {
		return -1;
	}

	bool lower = false;
	bool upper = false;
	bool digit = false;

	uint8_t password_index = 0;
	while (password_index < user->password_len) {
		char *byte = &user->password[password_index];
		if (*byte >= '0' && *byte <= '9') {
			digit = true;
		} else if (*byte >= 'a' && *byte <= 'z') {
			lower = true;
		} else if (*byte >= 'A' && *byte <= 'Z') {
			upper = true;
		}
		password_index++;
	}

	if (!lower || !upper || !digit) {
		debug("password contains lower %s upper %s digit %s \n", human_bool(lower), human_bool(upper), human_bool(digit));
		return -1;
	}

	return 0;
}

uint16_t user_insert(sqlite3 *database, user_t *user) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into user (id, username, password, signup_at, signin_at, permissions) "
										"values (randomblob(16), ?, ?, ?, ?, ?) returning id, permissions";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	uint8_t hash[32];
	sha256(user->password, user->password_len, &hash);
	time_t now = time(NULL);
	memset(user->permissions, 0x00, sizeof(*user->permissions));

	sqlite3_bind_text(stmt, 1, user->username, user->username_len, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, hash, sizeof(hash), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, now);
	sqlite3_bind_int64(stmt, 4, now);
	sqlite3_bind_blob(stmt, 5, user->permissions, sizeof(*user->permissions), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*user->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*user->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t *permissions = sqlite3_column_blob(stmt, 1);
		const size_t permissions_len = (size_t)sqlite3_column_bytes(stmt, 1);
		if (permissions_len != sizeof(*user->permissions)) {
			error("permissions length %zu does not match buffer length %zu\n", permissions_len, sizeof(*user->permissions));
			status = 500;
			goto cleanup;
		}
		memcpy(user->id, id, id_len);
		memcpy(user->permissions, permissions, permissions_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("username %.*s already taken\n", (int)user->username_len, user->username);
		status = 409;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t user_update(sqlite3 *database, user_t *user) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "update user set signin_at = ? "
										"where username = ? and password = ? returning id, permissions";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	uint8_t hash[32];
	sha256(user->password, user->password_len, &hash);
	time_t now = time(NULL);

	sqlite3_bind_int64(stmt, 1, now);
	sqlite3_bind_text(stmt, 2, user->username, user->username_len, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 3, hash, sizeof(hash), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*user->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*user->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t *permissions = sqlite3_column_blob(stmt, 1);
		const size_t permissions_len = (size_t)sqlite3_column_bytes(stmt, 1);
		if (permissions_len != sizeof(*user->permissions)) {
			error("permissions length %zu does not match buffer length %zu\n", permissions_len, sizeof(*user->permissions));
			status = 500;
			goto cleanup;
		}
		memcpy(user->id, id, id_len);
		memcpy(user->permissions, permissions, permissions_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("invalid password for %.*s\n", (int)user->username_len, user->username);
		status = 401;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t user_delete(sqlite3 *database, user_t *user) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "delete from user "
										"where id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user->id, sizeof(*user->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result != SQLITE_DONE) {
		status = database_error(database, result);
		goto cleanup;
	}

	if (sqlite3_changes(database) == 0) {
		warn("user %02x%02x not found\n", (*user->id)[0], (*user->id)[1]);
		status = 404;
		goto cleanup;
	}

	status = 0;

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void user_find(sqlite3 *database, request_t *request, response_t *response) {
	user_query_t query = {.limit = 16, .offset = 0};
	if (strnfind(request->search.ptr, request->search.len, "order=", "&", &query.order, &query.order_len, 16) == -1) {
		response->status = 400;
		return;
	}

	if (strnfind(request->search.ptr, request->search.len, "sort=", "", &query.sort, &query.sort_len, 8) == -1) {
		response->status = 400;
		return;
	}

	uint8_t users_len = 0;
	uint16_t status = user_select(database, &query, response, &users_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu users\n", users_len);
	response->status = 200;
}

void user_find_one(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((user_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((user_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	user_t user = {.id = &id};
	uint16_t status = user_existing(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = user_select_one(database, &user, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found user %02x%02x\n", (*user.id)[0], (*user.id)[1]);
	response->status = 200;
}

void user_signup(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	uint8_t permissions[4];
	user_t user = {.id = &id, .permissions = &permissions};
	if (request->body.len == 0 || user_parse(&user, request) == -1 || user_validate(&user) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = user_insert(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	char bwt[109];
	if (bwt_sign(&bwt, user.id, user.permissions) == -1) {
		response->status = 500;
		return;
	}

	header_write(response, "set-cookie:auth=%.*s;Path=/;Max-Age=%d;SameSite=Strict;HttpOnly;\r\n", (int)sizeof(bwt), bwt,
							 bwt_ttl);
	info("user %.*s signed up\n", (int)user.username_len, user.username);
	response->status = 201;
}

void user_signin(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	uint8_t permissions[4];
	user_t user = {.id = &id, .permissions = &permissions};
	if (request->body.len == 0 || user_parse(&user, request) == -1 || user_validate(&user) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = user_update(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	char bwt[109];
	if (bwt_sign(&bwt, user.id, user.permissions) == -1) {
		response->status = 500;
		return;
	}

	header_write(response, "set-cookie:auth=%.*s;Path=/;Max-Age=%d;SameSite=Strict;HttpOnly;\r\n", (int)sizeof(bwt), bwt,
							 bwt_ttl);
	info("user %.*s signed in\n", (int)user.username_len, user.username);
	response->status = 201;
}

void user_remove(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((user_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((user_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	user_t user = {.id = &id};
	uint16_t status = user_delete(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("deleted user %02x%02x\n", (*user.id)[0], (*user.id)[1]);
	response->status = 200;
}
