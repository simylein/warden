#include "radio.h"
#include "../lib/logger.h"
#include "database.h"
#include <sqlite3.h>
#include <stdint.h>
#include <string.h>

const char *radio_table = "radio";
const char *radio_schema = "create table radio ("
													 "id blob primary key, "
													 "frequency integer not null, "
													 "bandwidth integer not null, "
													 "coding_rate integer not null, "
													 "spreading_factor integer not null, "
													 "preamble_length integer not null, "
													 "tx_power integer not null, "
													 "sync_word integer not null, "
													 "checksum boolean not null, "
													 "captured_at timestamp not null, "
													 "uplink_id blob not null unique, "
													 "device_id blob not null, "
													 "foreign key (uplink_id) references uplink(id) on delete cascade, "
													 "foreign key (device_id) references device(id) on delete cascade"
													 ")";

uint16_t radio_insert(sqlite3 *database, radio_t *radio) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into radio (id, frequency, bandwidth, coding_rate, spreading_factor, "
										"preamble_length, tx_power, sync_word, checksum, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, (int)radio->frequency);
	sqlite3_bind_int(stmt, 2, (int)radio->bandwidth);
	sqlite3_bind_int(stmt, 3, radio->coding_rate);
	sqlite3_bind_int(stmt, 4, radio->spreading_factor);
	sqlite3_bind_int(stmt, 5, radio->preamble_length);
	sqlite3_bind_int(stmt, 6, radio->tx_power);
	sqlite3_bind_int(stmt, 7, radio->sync_word);
	sqlite3_bind_int(stmt, 8, radio->checksum);
	sqlite3_bind_int64(stmt, 9, radio->captured_at);
	sqlite3_bind_blob(stmt, 10, radio->uplink_id, sizeof(*radio->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 11, radio->device_id, sizeof(*radio->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*radio->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*radio->id));
			status = 500;
			goto cleanup;
		}
		memcpy(radio->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("radio is conflicting because %s\n", sqlite3_errmsg(database));
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
