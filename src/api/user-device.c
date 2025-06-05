#include "user-device.h"
#include "../lib/logger.h"
#include <sqlite3.h>
#include <stdint.h>
#include <stdlib.h>

const char *user_device_table = "user device";
const char *user_device_schema = "create table user_device ("
																 "user_id blob not null, "
																 "device_id blob not null, "
																 "primary key (user_id, device_id), "
																 "foreign key (user_id) references user(id) on delete cascade, "
																 "foreign key (device_id) references device(id) on delete cascade"
																 ")";

uint16_t user_device_insert(sqlite3 *database, user_device_t *user_device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into user_device (user_id, device_id) "
										"values (?, ?)";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user_device->user_id, sizeof(*user_device->user_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, user_device->device_id, sizeof(*user_device->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_DONE) {
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("user %4x device %4x are conflicting\n", *(uint32_t *)(*user_device->user_id), *(uint32_t *)(*user_device->device_id));
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
