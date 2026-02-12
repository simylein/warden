#include "user-device.h"
#include "../lib/base16.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include "user.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *user_device_file = "user-device";

const user_device_row_t user_device_row = {
		.user_id = 0,
		.device_id = 8,
		.size = 16,
};

uint16_t user_device_existing(octet_t *db, user_device_t *user_device) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select existing user %02x%02x device %02x%02x\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
				(*user_device->device_id)[0], (*user_device->device_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("user %02x%02x device %02x%02x not found\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
					 (*user_device->device_id)[0], (*user_device->device_id)[1]);
			status = 403;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[8] = (uint8_t (*)[8])octet_blob_read(db->row, user_device_row.user_id);
		uint8_t (*device_id)[8] = (uint8_t (*)[8])octet_blob_read(db->row, user_device_row.device_id);
		if (memcmp(user_id, user_device->user_id, sizeof(*user_device->user_id)) == 0 &&
				memcmp(device_id, user_device->device_id, sizeof(*user_device->device_id)) == 0) {
			status = 0;
			break;
		}
		offset += user_device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_device_select_by_user(octet_t *db, user_t *user, uint8_t *user_devices_len) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select user devices for user %02x%02x\n", (*user->id)[0], (*user->id)[1]);

	off_t offset = 0;
	uint16_t chunk_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->chunk[chunk_len], user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[8] = (uint8_t (*)[8])octet_blob_read(&db->chunk[chunk_len], user_device_row.user_id);
		if (memcmp(user_id, user->id, sizeof(*user->id)) == 0) {
			*user_devices_len += 1;
			chunk_len += user_device_row.size;
		}
		offset += user_device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_device_insert(octet_t *db, user_device_t *user_device) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert user %02x%02x device %02x%02x\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
				(*user_device->device_id)[0], (*user_device->device_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[8] = (uint8_t (*)[8])octet_blob_read(db->row, user_device_row.user_id);
		uint8_t (*device_id)[8] = (uint8_t (*)[8])octet_blob_read(db->row, user_device_row.device_id);
		if (memcmp(user_id, user_device->user_id, sizeof(*user_device->user_id)) == 0 &&
				memcmp(device_id, user_device->device_id, sizeof(*user_device->device_id)) == 0) {
			status = 409;
			warn("user %02x%02x device %02x%02x already exists\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
					 (*user_device->device_id)[0], (*user_device->device_id)[1]);
			goto cleanup;
		}
		offset += user_device_row.size;
	}

	offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - user_device_row.size, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint64_t user_id = octet_uint64_read(db->row, user_device_row.user_id);
		uint64_t user_device_user_id = octet_uint64_read((uint8_t *)user_device->user_id, 0);
		if (user_id <= user_device_user_id) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= user_device_row.size;
	}

	octet_blob_write(db->row, user_device_row.user_id, (uint8_t *)user_device->user_id, sizeof(*user_device->user_id));
	octet_blob_write(db->row, user_device_row.device_id, (uint8_t *)user_device->device_id, sizeof(*user_device->device_id));

	if (octet_row_write(&stmt, file, offset, db->row, user_device_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_device_delete(octet_t *db, user_device_t *user_device) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("delete user %02x%02x device %02x%02x\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
				(*user_device->device_id)[0], (*user_device->device_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("user %02x%02x device %02x%02x not found\n", (*user_device->user_id)[0], (*user_device->user_id)[1],
					 (*user_device->device_id)[0], (*user_device->device_id)[1]);
			status = 404;
			goto cleanup;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[8] = (uint8_t (*)[8])octet_blob_read(db->row, user_device_row.user_id);
		uint8_t (*device_id)[8] = (uint8_t (*)[8])octet_blob_read(db->row, user_device_row.device_id);
		if (memcmp(user_id, user_device->user_id, sizeof(*user_device->user_id)) == 0 &&
				memcmp(device_id, user_device->device_id, sizeof(*user_device->device_id)) == 0) {
			break;
		}
		offset += user_device_row.size;
	}

	off_t index = offset + user_device_row.size;
	while (index < stmt.stat.st_size) {
		if (octet_row_read(&stmt, file, index, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		if (octet_row_write(&stmt, file, index - user_device_row.size, db->row, user_device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		index += user_device_row.size;
	}

	if (octet_trunc(&stmt, file, stmt.stat.st_size - user_device_row.size)) {
		status = 500;
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
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

	uint8_t id[8];
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

	device_t device = {.id = (uint8_t (*)[8])request->body.ptr};
	status = device_existing(db, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	user_device_t user_device = {.user_id = user.id, .device_id = device.id};
	status = user_device_insert(db, &user_device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("created user %02x%02x device %02x%02x\n", (*user_device.user_id)[0], (*user_device.user_id)[1],
			 (*user_device.device_id)[0], (*user_device.device_id)[1]);
	response->status = 200;
}

void user_device_remove(octet_t *db, request_t *request, response_t *response) {
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

	uint8_t id[8];
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

	user_device_t user_device = {.user_id = user.id, .device_id = (uint8_t (*)[8])request->body.ptr};
	status = user_device_delete(db, &user_device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("deleted user %02x%02x device %02x%02x\n", (*user_device.user_id)[0], (*user_device.user_id)[1],
			 (*user_device.device_id)[0], (*user_device.device_id)[1]);
	response->status = 200;
}
