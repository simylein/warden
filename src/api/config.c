#include "config.h"
#include "../lib/logger.h"
#include "database.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *config_table = "config";
const char *config_schema = "create table config ("
														"id blob primary key, "
														"led_debug boolean not null, "
														"reading_enable boolean not null, "
														"metric_enable boolean not null, "
														"buffer_enable boolean not null, "
														"reading_interval integer not null, "
														"metric_interval integer not null, "
														"buffer_interval integer not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"device_id blob not null, "
														"foreign key (uplink_id) references uplink(id) on delete cascade, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";

uint16_t config_insert(sqlite3 *database, config_t *config) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into config (id, led_debug, reading_enable, metric_enable, buffer_enable, "
										"reading_interval, metric_interval, buffer_interval, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, config->led_debug);
	sqlite3_bind_int(stmt, 2, config->reading_enable);
	sqlite3_bind_int(stmt, 3, config->metric_enable);
	sqlite3_bind_int(stmt, 4, config->buffer_enable);
	sqlite3_bind_int(stmt, 5, config->reading_interval);
	sqlite3_bind_int(stmt, 6, config->metric_interval);
	sqlite3_bind_int(stmt, 7, config->buffer_interval);
	sqlite3_bind_int64(stmt, 8, config->captured_at);
	sqlite3_bind_blob(stmt, 9, config->uplink_id, sizeof(*config->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 10, config->device_id, sizeof(*config->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*config->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*config->id));
			status = 500;
			goto cleanup;
		}
		memcpy(config->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("config is conflicting because %s\n", sqlite3_errmsg(database));
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
