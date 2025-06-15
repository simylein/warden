#include "reading.h"
#include "../lib/logger.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *reading_table = "reading";
const char *reading_schema = "create table reading ("
														 "id blob primary key, "
														 "temperature real not null, "
														 "humidity real not null, "
														 "captured_at timestamp not null, "
														 "uplink_id blob not null, "
														 "foreign key (uplink_id) references uplink(id) on delete cascade"
														 ")";

uint16_t reading_insert(sqlite3 *database, reading_t *reading) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into reading (id, temperature, humidity, captured_at, uplink_id) "
										"values (randomblob(16), ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_double(stmt, 1, reading->temperature);
	sqlite3_bind_double(stmt, 2, reading->humidity);
	sqlite3_bind_int64(stmt, 3, reading->captured_at);
	sqlite3_bind_blob(stmt, 4, reading->uplink_id, sizeof(*reading->uplink_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*reading->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*reading->id));
			status = 500;
			goto cleanup;
		}
		memcpy(reading->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("reading is conflicting because %s\n", sqlite3_errmsg(database));
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
