#include "zone.h"
#include "../lib/logger.h"
#include "database.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *zone_table = "zone";
const char *zone_schema = "create table zone ("
													"id blob primary key, "
													"name text not null unique, "
													"color blob not null, "
													"created_at timestamp not null, "
													"updated_at timestamp"
													")";

uint16_t zone_insert(sqlite3 *database, zone_t *zone) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into zone (id, name, color, created_at) "
										"values (randomblob(16), ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, zone->name, zone->name_len, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, zone->color, sizeof(*zone->color), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, time(NULL));

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*zone->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*zone->id));
			status = 500;
			goto cleanup;
		}
		memcpy(zone->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("zone name %.*s already taken\n", (int)zone->name_len, zone->name);
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
