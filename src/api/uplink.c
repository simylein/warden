#include "uplink.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *uplink_table = "uplink";
const char *uplink_schema = "create table uplink ("
														"id blob primary key, "
														"kind integer not null, "
														"data blob not null, "
														"airtime real not null, "
														"frequency integer not null, "
														"bandwidth integer not null, "
														"rssi integer not null, "
														"snr real not null, "
														"sf integer not null, "
														"received_at datetime not null, "
														"device_id blob not null, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";

uint16_t uplink_select(sqlite3 *database, bwt_t *bwt, response_t *response, uint8_t *uplinks_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, kind, data, rssi, snr, sf, received_at, uplink.device_id from uplink "
										"join user_device on uplink.device_id = user_device.device_id "
										"where user_device.user_id = ?";
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
			if (id_len != sizeof(*((uplink_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t kind = (uint8_t)sqlite3_column_int(stmt, 1);
			const uint8_t *data = sqlite3_column_blob(stmt, 2);
			const size_t data_len = (size_t)sqlite3_column_bytes(stmt, 2);
			if (data_len > UINT8_MAX) {
				error("data length %zu exceeds buffer length %hhu\n", id_len, UINT8_MAX);
				status = 500;
				goto cleanup;
			}
			const int16_t rssi = (int16_t)sqlite3_column_int(stmt, 3);
			const double snr = sqlite3_column_double(stmt, 4);
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 5);
			const time_t received_at = (time_t)sqlite3_column_int64(stmt, 6);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 7);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
			if (device_id_len != sizeof(*((uplink_t *)0)->device_id)) {
				error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((uplink_t *)0)->device_id));
				status = 500;
				goto cleanup;
			}
			append_body(response, id, id_len);
			append_body(response, &kind, sizeof(kind));
			append_body(response, &data_len, sizeof(uint8_t));
			append_body(response, data, data_len);
			append_body(response, &(uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
			append_body(response, &(uint8_t[]){(uint8_t)(int8_t)(snr * 4)}, sizeof(uint8_t));
			append_body(response, &sf, sizeof(sf));
			append_body(response, &(uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
			append_body(response, device_id, device_id_len);
			*uplinks_len += 1;
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

uint16_t uplink_insert(sqlite3 *database, uplink_t *uplink) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into uplink (id, kind, data, airtime, frequency, bandwidth, rssi, snr, sf, received_at, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, uplink->kind);
	sqlite3_bind_blob(stmt, 2, uplink->data, uplink->data_len, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 3, (double)uplink->airtime / (16 * 1000));
	sqlite3_bind_int(stmt, 4, (int)uplink->frequency);
	sqlite3_bind_int(stmt, 5, (int)uplink->bandwidth);
	sqlite3_bind_int(stmt, 6, uplink->rssi);
	sqlite3_bind_double(stmt, 7, (double)uplink->snr / 4);
	sqlite3_bind_int(stmt, 8, uplink->sf);
	sqlite3_bind_int64(stmt, 9, uplink->received_at);
	sqlite3_bind_blob(stmt, 10, uplink->device_id, sizeof(*uplink->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*uplink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*uplink->id));
			status = 500;
			goto cleanup;
		}
		memcpy(uplink->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("uplink from device %04x is conflicting\n", *(uint32_t *)(*uplink->device_id));
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

void uplink_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uplinks_len;
	uint16_t status = uplink_select(database, bwt, response, &uplinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found %hhu uplinks\n", uplinks_len);
	response->status = 200;
}
