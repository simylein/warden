#include "buffer.h"
#include "../lib/logger.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

const char *buffer_table = "buffer";
const char *buffer_schema = "create table buffer ("
														"id blob primary key, "
														"delay integer not null, "
														"level integer not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"device_id blob not null, "
														"foreign key (uplink_id) references uplink(id) on delete cascade, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";

uint16_t buffer_insert(sqlite3 *database, buffer_t *buffer) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into buffer (id, delay, level, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_double(stmt, 1, buffer->delay);
	sqlite3_bind_double(stmt, 2, buffer->level);
	sqlite3_bind_int64(stmt, 3, buffer->captured_at);
	sqlite3_bind_blob(stmt, 4, buffer->uplink_id, sizeof(*buffer->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 5, buffer->device_id, sizeof(*buffer->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*buffer->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*buffer->id));
			status = 500;
			goto cleanup;
		}
		memcpy(buffer->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("buffer is conflicting because %s\n", sqlite3_errmsg(database));
		status = 409;
		goto cleanup;
	} else {
		error("failed to execute statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}
