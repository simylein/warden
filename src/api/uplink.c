#include "uplink.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "decode.h"
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

int uplink_parse(uplink_t *uplink, request_t *request) {
	uint16_t index = 0;

	if (request->body_len < index + sizeof(uplink->kind)) {
		return -1;
	}
	uplink->kind = (uint8_t)(*request->body)[index];
	index += sizeof(uplink->kind);

	if (request->body_len < index + sizeof(uplink->data_len)) {
		return -1;
	}
	uplink->data_len = (uint8_t)(*request->body)[index];
	index += sizeof(uplink->data_len);

	if (request->body_len < index + uplink->data_len) {
		return -1;
	}
	uplink->data = (uint8_t *)&(*request->body)[index];
	index += uplink->data_len;

	if (request->body_len < index + sizeof(uplink->airtime)) {
		return -1;
	}
	memcpy(&uplink->airtime, &(*request->body)[index], sizeof(uplink->airtime));
	uplink->airtime = ntoh16(uplink->airtime);
	index += sizeof(uplink->airtime);

	if (request->body_len < index + sizeof(uplink->frequency)) {
		return -1;
	}
	memcpy(&uplink->frequency, &(*request->body)[index], sizeof(uplink->frequency));
	uplink->frequency = ntoh32(uplink->frequency);
	index += sizeof(uplink->frequency);

	if (request->body_len < index + sizeof(uplink->bandwidth)) {
		return -1;
	}
	memcpy(&uplink->bandwidth, &(*request->body)[index], sizeof(uplink->bandwidth));
	uplink->bandwidth = ntoh32(uplink->bandwidth);
	index += sizeof(uplink->bandwidth);

	if (request->body_len < index + sizeof(uplink->rssi)) {
		return -1;
	}
	memcpy(&uplink->rssi, &(*request->body)[index], sizeof(uplink->rssi));
	uplink->rssi = (int16_t)ntoh16((uint16_t)uplink->rssi);
	index += sizeof(uplink->rssi);

	if (request->body_len < index + sizeof(uplink->snr)) {
		return -1;
	}
	uplink->snr = (int8_t)(*request->body)[index];
	index += sizeof(uplink->snr);

	if (request->body_len < index + sizeof(uplink->sf)) {
		return -1;
	}
	uplink->sf = (uint8_t)(*request->body)[index];
	index += sizeof(uplink->sf);

	if (request->body_len < index + sizeof(uplink->received_at)) {
		return -1;
	}
	memcpy(&uplink->received_at, &(*request->body)[index], sizeof(uplink->received_at));
	uplink->received_at = (time_t)ntoh64((uint64_t)uplink->received_at);
	index += sizeof(uplink->received_at);

	if (request->body_len < index + sizeof(*uplink->device_id)) {
		return -1;
	}
	uplink->device_id = (uint8_t (*)[16])(&(*request->body)[index]);
	index += sizeof(*uplink->device_id);

	if (request->body_len != index) {
		return -1;
	}

	return 0;
}

int uplink_validate(uplink_t *uplink) {
	if (uplink->frequency < 400 * 1000 * 1000 || uplink->frequency > 500 * 1000 * 1000) {
		return -1;
	}

	if (uplink->bandwidth < 7800 || uplink->bandwidth > 500 * 1000) {
		return -1;
	}

	if (uplink->rssi < -192 || uplink->rssi > 16) {
		return -1;
	}

	if (uplink->snr < -96 || uplink->snr > 48) {
		return -1;
	}

	if (uplink->sf < 6 || uplink->sf > 12) {
		return -1;
	}

	return 0;
}

uint16_t uplink_select(sqlite3 *database, bwt_t *bwt, uplink_query_t *query, response_t *response, uint8_t *uplinks_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, kind, data, rssi, snr, sf, received_at, uplink.device_id from uplink "
										"join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"order by received_at desc "
										"limit ? offset ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, query->limit);
	sqlite3_bind_int(stmt, 3, (int)query->offset);

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
		warn("uplink from device %02x%02x is conflicting\n", (*uplink->device_id)[0], (*uplink->device_id)[1]);
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
	uplink_query_t query = {.limit = 16, .offset = 0};
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uplinks_len;
	uint16_t status = uplink_select(database, bwt, &query, response, &uplinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found %hhu uplinks\n", uplinks_len);
	response->status = 200;
}

void uplink_create(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	uplink_t uplink = {.id = &id};
	if (request->body_len == 0 || uplink_parse(&uplink, request) == -1 || uplink_validate(&uplink) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = uplink_insert(database, &uplink);
	if (status != 0) {
		response->status = status;
		return;
	}

	if (decode(database, &uplink) == -1) {
		response->status = 500;
		return;
	}

	info("created uplink %02x%02x\n", (*uplink.id)[0], (*uplink.id)[1]);
	response->status = 201;
}
