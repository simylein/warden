#include "user-zone.h"
#include "../lib/base16.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "user.h"
#include "zone.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char *user_zone_file = "user-zone";

const user_zone_row_t user_zone_row = {
		.user_id = 0,
		.zone_id = 16,
		.size = 32,
};

uint16_t user_zone_existing(octet_t *db, user_zone_t *user_zone) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select existing user %02x%02x zone %02x%02x\n", (*user_zone->user_id)[0], (*user_zone->user_id)[1],
				(*user_zone->zone_id)[0], (*user_zone->zone_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("user %02x%02x zone %02x%02x not found\n", (*user_zone->user_id)[0], (*user_zone->user_id)[1],
					 (*user_zone->zone_id)[0], (*user_zone->zone_id)[1]);
			status = 403;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}

		uint8_t (*user_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_zone_row.user_id);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_zone_row.zone_id);
		if (memcmp(user_id, user_zone->user_id, sizeof(*user_zone->user_id)) == 0 &&
				memcmp(zone_id, user_zone->zone_id, sizeof(*user_zone->zone_id)) == 0) {
			status = 0;
			break;
		}

		offset += user_zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_zone_select_by_user(octet_t *db, user_t *user, uint8_t *user_zones_len) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select user zones for user %02x%02x\n", (*user->id)[0], (*user->id)[1]);

	off_t offset = 0;
	uint16_t chunk_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->chunk[chunk_len], user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->chunk[chunk_len], user_zone_row.user_id);
		if (memcmp(user_id, user->id, sizeof(*user->id)) == 0) {
			*user_zones_len += 1;
			chunk_len += user_zone_row.size;
		}
		offset += user_zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_zone_insert(octet_t *db, user_zone_t *user_zone) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert user %02x%02x zone %02x%02x\n", (*user_zone->user_id)[0], (*user_zone->user_id)[1], (*user_zone->zone_id)[0],
				(*user_zone->zone_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_zone_row.user_id);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_zone_row.zone_id);
		if (memcmp(user_id, user_zone->user_id, sizeof(*user_zone->user_id)) == 0 &&
				memcmp(zone_id, user_zone->zone_id, sizeof(*user_zone->zone_id)) == 0) {
			status = 409;
			warn("user %02x%02x zone %02x%02x already exists\n", (*user_zone->user_id)[0], (*user_zone->user_id)[1],
					 (*user_zone->zone_id)[0], (*user_zone->zone_id)[1]);
			goto cleanup;
		}
		offset += user_zone_row.size;
	}

	offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - user_zone_row.size, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint64_t user_id = octet_uint64_read(db->row, user_zone_row.user_id);
		uint64_t user_zone_user_id = octet_uint64_read((uint8_t *)user_zone->user_id, 0);
		if (user_id <= user_zone_user_id) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= user_zone_row.size;
	}

	octet_blob_write(db->row, user_zone_row.user_id, (uint8_t *)user_zone->user_id, sizeof(*user_zone->user_id));
	octet_blob_write(db->row, user_zone_row.zone_id, (uint8_t *)user_zone->zone_id, sizeof(*user_zone->zone_id));

	if (octet_row_write(&stmt, file, offset, db->row, user_zone_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t user_zone_delete(octet_t *db, user_zone_t *user_zone) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("delete user %02x%02x zone %02x%02x\n", (*user_zone->user_id)[0], (*user_zone->user_id)[1], (*user_zone->zone_id)[0],
				(*user_zone->zone_id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("user %02x%02x zone %02x%02x not found\n", (*user_zone->user_id)[0], (*user_zone->user_id)[1],
					 (*user_zone->zone_id)[0], (*user_zone->zone_id)[1]);
			status = 404;
			goto cleanup;
		}
		if (octet_row_read(&stmt, file, offset, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*user_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_zone_row.user_id);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, user_zone_row.zone_id);
		if (memcmp(user_id, user_zone->user_id, sizeof(*user_zone->user_id)) == 0 &&
				memcmp(zone_id, user_zone->zone_id, sizeof(*user_zone->zone_id)) == 0) {
			break;
		}
		offset += user_zone_row.size;
	}

	off_t index = offset + user_zone_row.size;
	while (index < stmt.stat.st_size) {
		if (octet_row_read(&stmt, file, index, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		if (octet_row_write(&stmt, file, index - user_zone_row.size, db->row, user_zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		index += user_zone_row.size;
	}

	if (octet_trunc(&stmt, file, stmt.stat.st_size - user_zone_row.size)) {
		status = 500;
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void user_zone_create(octet_t *db, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((zone_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((zone_t *)0)->id) * 2);
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

	if (request->body.len != sizeof(*((zone_t *)0)->id)) {
		response->status = 400;
		return;
	}

	zone_t zone = {.id = (uint8_t (*)[16])request->body.ptr};
	status = zone_existing(db, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	user_zone_t user_zone = {.user_id = user.id, .zone_id = zone.id};
	status = user_zone_insert(db, &user_zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("created user zone %02x%02x\n", (*user_zone.zone_id)[0], (*user_zone.zone_id)[1]);
	response->status = 200;
}

void user_zone_remove(octet_t *db, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((zone_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((zone_t *)0)->id) * 2);
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

	if (request->body.len != sizeof(*((zone_t *)0)->id)) {
		response->status = 400;
		return;
	}

	user_zone_t user_zone = {.user_id = user.id, .zone_id = (uint8_t (*)[16])request->body.ptr};
	status = user_zone_delete(db, &user_zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("deleted user %02x%02x zone %02x%02x\n", (*user_zone.user_id)[0], (*user_zone.user_id)[1], (*user_zone.zone_id)[0],
			 (*user_zone.zone_id)[1]);
	response->status = 200;
}
