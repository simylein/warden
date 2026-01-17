#include "user-device.h"
#include "../lib/base16.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "database.h"
#include "device.h"
#include <fcntl.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *user_device_table = "user_device";
const char *user_device_schema = "create table user_device ("
																 "user_id blob not null, "
																 "device_id blob not null, "
																 "primary key (user_id, device_id), "
																 "foreign key (user_id) references user(id) on delete cascade, "
																 "foreign key (device_id) references device(id) on delete cascade"
																 ")";

const user_device_row_t user_device_row = {
		.user_id = 0,
		.device_id = 16,
		.size = 32,
};

uint16_t user_device_insert(octet_t *db, user_device_t *user_device) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/user-device.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("insert user %02x%02x device %02x%02x\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
				(*user_device->device_id)[0], (*user_device->device_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->buffer, user_device_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t (*user_id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, user_device_row.user_id);
		uint8_t (*device_id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, user_device_row.device_id);
		if (memcmp(user_id, user_device->user_id, sizeof(*user_device->user_id)) == 0 &&
				memcmp(device_id, user_device->device_id, sizeof(*user_device->device_id)) == 0) {
			status = 409;
			warn("user %02x%02x device %02x%02x already exists\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
					 (*user_device->device_id)[0], (*user_device->device_id)[1]);
			goto cleanup;
		}
		offset += user_device_row.size;
	}

	octet_blob_write(db->buffer, user_device_row.user_id, (uint8_t *)user_device->user_id, sizeof(*user_device->user_id));
	octet_blob_write(db->buffer, user_device_row.device_id, (uint8_t *)user_device->device_id, sizeof(*user_device->device_id));

	offset = stmt.stat.st_size;
	if (octet_row_write(&stmt, file, offset, db->buffer, user_device_row.size) == -1) {
		status = 500;
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_device_delete(sqlite3 *database, user_device_t *user_device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "delete from user_device "
										"where user_id = ? and device_id = ?";
	debug("delete user %02x%02x device %02x%02x\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
				(*user_device->device_id)[0], (*user_device->device_id)[1]);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user_device->user_id, sizeof(*user_device->user_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, user_device->device_id, sizeof(*user_device->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result != SQLITE_DONE) {
		status = database_error(database, result);
		goto cleanup;
	}

	if (sqlite3_changes(database) == 0) {
		warn("user device %02x%02x not found\n", (*user_device->device_id)[0], (*user_device->device_id)[1]);
		status = 404;
		goto cleanup;
	}

	status = 0;

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void user_device_create(octet_t *db, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
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

	if (request->body.len != sizeof(*((device_t *)0)->id)) {
		response->status = 400;
		return;
	}

	user_device_t user_device = {.user_id = user.id, .device_id = (uint8_t (*)[16])request->body.ptr};
	status = user_device_insert(db, &user_device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("created user device %02x%02x\n", (*user_device.device_id)[0], (*user_device.device_id)[1]);
	response->status = 200;
}

void user_device_remove(octet_t *db, sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
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

	if (request->body.len != sizeof(*((device_t *)0)->id)) {
		response->status = 400;
		return;
	}

	user_device_t user_device = {.user_id = user.id, .device_id = (uint8_t (*)[16])request->body.ptr};
	status = user_device_delete(database, &user_device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("deleted user device %02x%02x\n", (*user_device.device_id)[0], (*user_device.device_id)[1]);
	response->status = 200;
}
