#include "user-zone.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "user.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
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
		status = 500;
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
			status = 500;
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
		status = 500;
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
			status = 500;
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
