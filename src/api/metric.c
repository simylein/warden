#include "metric.h"
#include "../lib/logger.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *metric_table = "metric";
const char *metric_schema = "create table metric ("
														"id blob primary key, "
														"photovoltaic real not null, "
														"battery real not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"foreign key (uplink_id) references uplink(id) on delete cascade"
														")";

uint16_t metric_insert(sqlite3 *database, metric_t *metric) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into metric (id, photovoltaic, battery, captured_at, uplink_id) "
										"values (randomblob(16), ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_double(stmt, 1, metric->photovoltaic);
	sqlite3_bind_double(stmt, 2, metric->battery);
	sqlite3_bind_int64(stmt, 3, metric->captured_at);
	sqlite3_bind_blob(stmt, 4, metric->uplink_id, sizeof(*metric->uplink_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*metric->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*metric->id));
			status = 500;
			goto cleanup;
		}
		memcpy(metric->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("metric is conflicting because %s\n", sqlite3_errmsg(database));
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
