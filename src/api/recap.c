#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>

uint16_t recap_select(sqlite3 *database, bwt_t *bwt, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"device.id, device.name, device.type, "
										"min(reading.temperature), max(reading.temperature), "
										"min(reading.humidity), max(reading.humidity), "
										"reading.captured_at "
										"from device "
										"join user_device on user_device.device_id = device.id and user_device.user_id = ? "
										"left join reading on reading.device_id = device.id "
										"where reading.captured_at >= strftime('%s', 'now', '-6 days', 'start of day') "
										"group by device.id, date(reading.captured_at, 'unixepoch')";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const uint8_t *id = sqlite3_column_blob(stmt, 0);
			const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
			if (id_len != sizeof(*((device_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((device_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *name = sqlite3_column_text(stmt, 1);
			const size_t name_len = (size_t)sqlite3_column_bytes(stmt, 1);
			const uint8_t *type = sqlite3_column_text(stmt, 2);
			const size_t type_len = (size_t)sqlite3_column_bytes(stmt, 2);
			const double temperature_min = sqlite3_column_double(stmt, 3);
			const int temperature_min_type = sqlite3_column_type(stmt, 3);
			const double temperature_max = sqlite3_column_double(stmt, 4);
			const int temperature_max_type = sqlite3_column_type(stmt, 4);
			const double humidity_min = sqlite3_column_double(stmt, 5);
			const int humidity_min_type = sqlite3_column_type(stmt, 5);
			const double humidity_max = sqlite3_column_double(stmt, 6);
			const int humidity_max_type = sqlite3_column_type(stmt, 6);
			const uint64_t captured_at = (uint64_t)sqlite3_column_int64(stmt, 7);
			const int captured_at_type = sqlite3_column_type(stmt, 7);
			append_body(response, id, id_len);
			append_body(response, name, name_len);
			append_body(response, (char[]){0x00}, sizeof(char));
			append_body(response, type, type_len);
			append_body(response, (char[]){0x00}, sizeof(char));
			append_body(response, (char[]){temperature_min_type != SQLITE_NULL}, sizeof(char));
			if (temperature_min_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(int16_t)(temperature_min * 100))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){temperature_max_type != SQLITE_NULL}, sizeof(char));
			if (temperature_max_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(int16_t)(temperature_max * 100))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){humidity_min_type != SQLITE_NULL}, sizeof(char));
			if (humidity_min_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(humidity_min * 100))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){humidity_max_type != SQLITE_NULL}, sizeof(char));
			if (humidity_max_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(humidity_max * 100))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){captured_at_type != SQLITE_NULL}, sizeof(char));
			if (captured_at_type != SQLITE_NULL) {
				append_body(response, (uint64_t[]){hton64(captured_at)}, sizeof(captured_at));
			}
		} else if (result == SQLITE_DONE) {
			status = 0;
			break;
		} else {
			error("failed to execute statement because %s\n", sqlite3_errmsg(database));
			status = 500;
			goto cleanup;
		}
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void recap_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint16_t status = recap_select(database, bwt, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found recap\n");
	response->status = 200;
}
