#include "user.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/config.h"
#include "../lib/endian.h"
#include "../lib/format.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/sha256.h"
#include "database.h"
#include <fcntl.h>
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

const user_row_t user_row = {
		.id = 0,
		.username_len = 16,
		.username = 17,
		.password = 33,
		.signup_at = 65,
		.signin_at = 73,
		.permissions = 81,
		.size = 85,
};

int user_rowcmp(uint8_t *alpha, uint8_t *bravo, user_query_t *query) {
	if (query->order_len == 2 && memcmp(query->order, "id", query->order_len) == 0) {
		uint64_t id_alpha = octet_uint64_read(alpha, user_row.id);
		uint64_t id_bravo = octet_uint64_read(bravo, user_row.id);
		int result = (id_alpha > id_bravo) - (id_alpha < id_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 8 && memcmp(query->order, "username", query->order_len) == 0) {
		uint8_t username_len_alpha = octet_uint8_read(alpha, user_row.username_len);
		char *username_alpha = octet_text_read(alpha, user_row.username);
		uint8_t username_len_bravo = octet_uint8_read(bravo, user_row.username_len);
		char *username_bravo = octet_text_read(bravo, user_row.username);
		int result = memcmp(username_alpha, username_bravo,
												username_len_alpha < username_len_bravo ? username_len_alpha : username_len_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 8 && memcmp(query->order, "signupAt", query->order_len) == 0) {
		time_t signup_at_alpha = (time_t)octet_uint64_read(alpha, user_row.signup_at);
		time_t signup_at_bravo = (time_t)octet_uint64_read(bravo, user_row.signup_at);
		int result = (signup_at_alpha > signup_at_bravo) - (signup_at_alpha < signup_at_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 8 && memcmp(query->order, "signinAt", query->order_len) == 0) {
		time_t signin_at_alpha = (time_t)octet_uint64_read(alpha, user_row.signin_at);
		time_t signin_at_bravo = (time_t)octet_uint64_read(bravo, user_row.signin_at);
		int result = (signin_at_alpha > signin_at_bravo) - (signin_at_alpha < signin_at_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	return 0;
}

uint16_t user_existing(octet_t *db, user_t *user) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/user.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("select existing user %02x%02x\n", (*user->id)[0], (*user->id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("user %02x%02x not found\n", (*user->id)[0], (*user->id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_row.id);
		if (memcmp(id, user->id, sizeof(*user->id)) == 0) {
			status = 0;
			break;
		}
		offset += user_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_select(octet_t *db, user_query_t *query, response_t *response, uint8_t *users_len) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/user.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	if (stmt.stat.st_size > db->table_len) {
		error("file length %zu exceeds buffer length %u\n", (size_t)stmt.stat.st_size, db->table_len);
		status = 500;
		goto cleanup;
	}

	debug("select users order by %.*s:%.*s\n", (int)query->order_len, query->order, (int)query->sort_len, query->sort);

	off_t offset = 0;
	uint32_t table_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->table[table_len], user_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		table_len += user_row.size;
		offset += user_row.size;
	}

	if (table_len >= user_row.size * 2) {
		for (uint8_t index = 0; index < table_len / user_row.size - 1; index++) {
			for (uint8_t ind = index + 1; ind < table_len / user_row.size; ind++) {
				if (user_rowcmp(&db->table[index * user_row.size], &db->table[ind * user_row.size], query) > 0) {
					memcpy(db->row, &db->table[index * user_row.size], user_row.size);
					memcpy(&db->table[index * user_row.size], &db->table[ind * user_row.size], user_row.size);
					memcpy(&db->table[ind * user_row.size], db->row, user_row.size);
				}
			}
		}
	}

	uint32_t index = user_row.size * query->offset;
	while (true) {
		if (index >= table_len || *users_len >= query->limit) {
			status = 0;
			break;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], user_row.id);
		uint8_t username_len = octet_uint8_read(&db->table[index], user_row.username_len);
		char *username = octet_text_read(&db->table[index], user_row.username);
		time_t signup_at = (time_t)octet_uint64_read(&db->table[index], user_row.signup_at);
		time_t signin_at = (time_t)octet_uint64_read(&db->table[index], user_row.signin_at);
		uint8_t (*permissions)[4] = (uint8_t (*)[4])octet_blob_read(&db->table[index], user_row.permissions);
		body_write(response, id, sizeof(*id));
		body_write(response, username, username_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, (uint64_t[]){hton64((uint64_t)signup_at)}, sizeof(signup_at));
		body_write(response, (uint64_t[]){hton64((uint64_t)signin_at)}, sizeof(signin_at));
		body_write(response, permissions, sizeof(*permissions));
		*users_len += 1;
		index += user_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_select_one(octet_t *db, user_t *user, response_t *response) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/user.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("select user %02x%02x\n", (*user->id)[0], (*user->id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_row.id);
		if (memcmp(id, user->id, sizeof(*user->id)) == 0) {
			uint8_t username_len = octet_uint8_read(db->row, user_row.username_len);
			char *username = octet_text_read(db->row, user_row.username);
			time_t signup_at = (time_t)octet_uint64_read(db->row, user_row.signup_at);
			time_t signin_at = (time_t)octet_uint64_read(db->row, user_row.signin_at);
			uint8_t (*permissions)[4] = (uint8_t (*)[4])octet_blob_read(db->row, user_row.permissions);
			body_write(response, id, sizeof(*id));
			body_write(response, username, username_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint64_t[]){hton64((uint64_t)signup_at)}, sizeof(signup_at));
			body_write(response, (uint64_t[]){hton64((uint64_t)signin_at)}, sizeof(signin_at));
			body_write(response, permissions, sizeof(*permissions));
			status = 0;
			break;
		}
		offset += user_row.size;
	}

cleanup:
	octet_close(&stmt, file);
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

uint16_t user_insert(octet_t *db, user_t *user) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*user->id); index++) {
		(*user->id)[index] = (uint8_t)(rand() % 0xff);
	}

	char file[128];
	if (sprintf(file, "%s/user.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("insert user %.*s signup at %lu\n", user->username_len, user->username, *user->signup_at);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t username_len = octet_uint8_read(db->row, user_row.username_len);
		char *username = octet_text_read(db->row, user_row.username);
		if (username_len == user->username_len && memcmp(username, user->username, user->username_len) == 0) {
			status = 409;
			warn("username %.*s already taken\n", user->username_len, user->username);
			goto cleanup;
		}
		offset += user_row.size;
	}

	uint8_t hash[32];
	sha256(user->password, user->password_len, &hash);
	memset(user->permissions, 0x00, sizeof(*user->permissions));

	octet_blob_write(db->row, user_row.id, (uint8_t *)user->id, sizeof(*user->id));
	octet_uint8_write(db->row, user_row.username_len, user->username_len);
	octet_text_write(db->row, user_row.username, user->username, user->username_len);
	octet_blob_write(db->row, user_row.password, hash, sizeof(hash));
	octet_uint64_write(db->row, user_row.signup_at, (uint64_t)*user->signup_at);
	octet_uint64_write(db->row, user_row.signin_at, (uint64_t)*user->signin_at);
	octet_blob_write(db->row, user_row.permissions, (uint8_t *)user->permissions, sizeof(*user->permissions));

	offset = stmt.stat.st_size;
	if (octet_row_write(&stmt, file, offset, db->row, user_row.size) == -1) {
		status = 500;
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_update(octet_t *db, user_t *user) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/user.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("update user %02x%02x signin at %lu\n", (*user->id)[0], (*user->id)[1], *user->signin_at);

	uint8_t hash[32];
	sha256(user->password, user->password_len, &hash);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("invalid password for %.*s\n", user->username_len, user->username);
			status = 401;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_row.id);
		uint8_t username_len = octet_uint8_read(db->row, user_row.username_len);
		char *username = octet_text_read(db->row, user_row.username);
		uint8_t (*password)[32] = (uint8_t (*)[32])octet_blob_read(db->row, user_row.password);
		uint8_t (*permissions)[4] = (uint8_t (*)[4])octet_blob_read(db->row, user_row.permissions);
		if (username_len == user->username_len && memcmp(username, user->username, user->username_len) == 0 &&
				memcmp(password, hash, sizeof(hash)) == 0) {
			octet_uint64_write(db->row, user_row.signin_at, (uint64_t)*user->signin_at);
			if (octet_row_write(&stmt, file, offset, db->row, user_row.size) == -1) {
				status = 500;
				goto cleanup;
			}
			memcpy(user->id, id, sizeof(*id));
			memcpy(user->permissions, permissions, sizeof(*permissions));
			status = 0;
			break;
		}
		offset += user_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_delete(sqlite3 *database, user_t *user) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "delete from user "
										"where id = ?";
	debug("delete user %02x%02x\n", (*user->id)[0], (*user->id)[1]);

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

void user_find(octet_t *db, request_t *request, response_t *response) {
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
	uint16_t status = user_select(db, &query, response, &users_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu users\n", users_len);
	response->status = 200;
}

void user_find_one(octet_t *db, request_t *request, response_t *response) {
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
	uint16_t status = user_existing(db, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = user_select_one(db, &user, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found user %02x%02x\n", (*user.id)[0], (*user.id)[1]);
	response->status = 200;
}

void user_profile(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	user_t user = {.id = &bwt->id};
	uint16_t status = user_existing(db, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = user_select_one(db, &user, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found profile %02x%02x\n", (*user.id)[0], (*user.id)[1]);
	response->status = 200;
}

void user_signup(octet_t *db, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	time_t now = time(NULL);
	uint8_t permissions[4];
	user_t user = {.id = &id, .permissions = &permissions, .signup_at = &now, .signin_at = &now};
	if (request->body.len == 0 || user_parse(&user, request) == -1 || user_validate(&user) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = user_insert(db, &user);
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

void user_signin(octet_t *db, request_t *request, response_t *response) {
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

	user.signin_at = (time_t[]){time(NULL)};
	uint16_t status = user_update(db, &user);
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
