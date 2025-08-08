#include "downlink.h"
#include "../lib/base16.h"
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

const char *downlink_table = "downlink";
const char *downlink_schema = "create table downlink ("
															"id blob primary key, "
															"kind integer not null, "
															"data blob not null, "
															"airtime real not null, "
															"frequency integer not null, "
															"bandwidth integer not null, "
															"tx_power integer not null, "
															"sf integer not null, "
															"sent_at datetime not null, "
															"device_id blob not null, "
															"foreign key (device_id) references device(id) on delete cascade"
															")";

uint16_t downlink_existing(sqlite3 *database, bwt_t *bwt, downlink_t *downlink) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"downlink.id, "
										"user_device.device_id "
										"from downlink "
										"left join user_device on user_device.device_id = downlink.device_id and user_device.user_id = ? "
										"where downlink.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, downlink->id, sizeof(*downlink->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*downlink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*downlink->id));
			status = 500;
			goto cleanup;
		}
		const int user_device_device_id_type = sqlite3_column_type(stmt, 1);
		if (user_device_device_id_type == SQLITE_NULL) {
			status = 403;
			goto cleanup;
		}
		memcpy(downlink->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("downlink %02x%02x not found\n", (*downlink->id)[0], (*downlink->id)[1]);
		status = 404;
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

uint16_t downlink_select(sqlite3 *database, bwt_t *bwt, downlink_query_t *query, response_t *response, uint8_t *downlinks_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, kind, data, tx_power, sf, sent_at, downlink.device_id from downlink "
										"join user_device on user_device.device_id = downlink.device_id and user_device.user_id = ? "
										"order by sent_at desc "
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
			if (id_len != sizeof(*((downlink_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((downlink_t *)0)->id));
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
			const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 3);
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 4);
			const time_t sent_at = (time_t)sqlite3_column_int64(stmt, 5);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 6);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 6);
			if (device_id_len != sizeof(*((downlink_t *)0)->device_id)) {
				error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((downlink_t *)0)->device_id));
				status = 500;
				goto cleanup;
			}
			append_body(response, id, id_len);
			append_body(response, &kind, sizeof(kind));
			append_body(response, &data_len, sizeof(uint8_t));
			append_body(response, data, data_len);
			append_body(response, &tx_power, sizeof(tx_power));
			append_body(response, &sf, sizeof(sf));
			append_body(response, &(uint64_t[]){hton64((uint64_t)sent_at)}, sizeof(sent_at));
			append_body(response, device_id, device_id_len);
			*downlinks_len += 1;
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

uint16_t downlink_select_one(sqlite3 *database, bwt_t *bwt, downlink_t *downlink, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"downlink.id, downlink.kind, downlink.data, "
										"downlink.airtime, downlink.frequency, downlink.bandwidth, "
										"downlink.tx_power, downlink.sf, "
										"downlink.sent_at, downlink.device_id "
										"from downlink "
										"join user_device on user_device.device_id = downlink.device_id and user_device.user_id = ? "
										"where downlink.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, downlink->id, sizeof(*downlink->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*downlink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*downlink->id));
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
		const double airtime = sqlite3_column_double(stmt, 3);
		const uint32_t frequency = (uint32_t)sqlite3_column_int(stmt, 4);
		const uint32_t bandwidth = (uint32_t)sqlite3_column_int(stmt, 5);
		const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 6);
		const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 7);
		const time_t sent_at = (time_t)sqlite3_column_int64(stmt, 8);
		const uint8_t *device_id = sqlite3_column_blob(stmt, 9);
		const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 9);
		if (device_id_len != sizeof(*((downlink_t *)0)->device_id)) {
			error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((downlink_t *)0)->device_id));
			status = 500;
			goto cleanup;
		}
		append_body(response, id, id_len);
		append_body(response, &kind, sizeof(kind));
		append_body(response, &data_len, sizeof(uint8_t));
		append_body(response, data, data_len);
		append_body(response, &(uint16_t[]){hton16((uint16_t)(airtime * 16 * 1000))}, sizeof(uint16_t));
		append_body(response, &(uint32_t[]){hton32(frequency)}, sizeof(frequency));
		append_body(response, &(uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
		append_body(response, &tx_power, sizeof(tx_power));
		append_body(response, &sf, sizeof(sf));
		append_body(response, &(uint64_t[]){hton64((uint64_t)sent_at)}, sizeof(sent_at));
		append_body(response, device_id, device_id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("downlink %02x%02x not found\n", (*downlink->id)[0], (*downlink->id)[1]);
		status = 404;
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

int downlink_parse(downlink_t *downlink, request_t *request) {
	uint16_t index = 0;

	if (request->body_len < index + sizeof(downlink->kind)) {
		debug("missing kind on downlink\n");
		return -1;
	}
	downlink->kind = (uint8_t)(*request->body)[index];
	index += sizeof(downlink->kind);

	if (request->body_len < index + sizeof(downlink->data_len)) {
		debug("missing data len on downlink\n");
		return -1;
	}
	downlink->data_len = (uint8_t)(*request->body)[index];
	index += sizeof(downlink->data_len);

	if (request->body_len < index + downlink->data_len) {
		debug("missing data on downlink\n");
		return -1;
	}
	downlink->data = (uint8_t *)&(*request->body)[index];
	index += downlink->data_len;

	if (request->body_len < index + sizeof(downlink->airtime)) {
		debug("missing airtime on downlink\n");
		return -1;
	}
	memcpy(&downlink->airtime, &(*request->body)[index], sizeof(downlink->airtime));
	downlink->airtime = ntoh16(downlink->airtime);
	index += sizeof(downlink->airtime);

	if (request->body_len < index + sizeof(downlink->frequency)) {
		debug("missing frequency on downlink\n");
		return -1;
	}
	memcpy(&downlink->frequency, &(*request->body)[index], sizeof(downlink->frequency));
	downlink->frequency = ntoh32(downlink->frequency);
	index += sizeof(downlink->frequency);

	if (request->body_len < index + sizeof(downlink->bandwidth)) {
		debug("missing bandwidth on downlink\n");
		return -1;
	}
	memcpy(&downlink->bandwidth, &(*request->body)[index], sizeof(downlink->bandwidth));
	downlink->bandwidth = ntoh32(downlink->bandwidth);
	index += sizeof(downlink->bandwidth);

	if (request->body_len < index + sizeof(downlink->tx_power)) {
		debug("missing tx power on downlink\n");
		return -1;
	}
	downlink->tx_power = (uint8_t)(*request->body)[index];
	index += sizeof(downlink->tx_power);

	if (request->body_len < index + sizeof(downlink->sf)) {
		debug("missing sf on downlink\n");
		return -1;
	}
	downlink->sf = (uint8_t)(*request->body)[index];
	index += sizeof(downlink->sf);

	if (request->body_len < index + sizeof(downlink->sent_at)) {
		debug("missing sent at on downlink\n");
		return -1;
	}
	memcpy(&downlink->sent_at, &(*request->body)[index], sizeof(downlink->sent_at));
	downlink->sent_at = (time_t)ntoh64((uint64_t)downlink->sent_at);
	index += sizeof(downlink->sent_at);

	if (request->body_len < index + sizeof(*downlink->device_id)) {
		debug("missing device id on downlink\n");
		return -1;
	}
	downlink->device_id = (uint8_t (*)[16])(&(*request->body)[index]);
	index += sizeof(*downlink->device_id);

	if (request->body_len != index) {
		debug("body len %zu does not match index %hu\n", request->body_len, index);
		return -1;
	}

	return 0;
}

int downlink_validate(downlink_t *downlink) {
	if (downlink->airtime < 128) {
		debug("invalid airtime %hu on downlink\n", downlink->airtime);
		return -1;
	}

	if (downlink->frequency < 400 * 1000 * 1000 || downlink->frequency > 500 * 1000 * 1000) {
		debug("invalid frequency %u on downlink\n", downlink->frequency);
		return -1;
	}

	if (downlink->bandwidth < 7800 || downlink->bandwidth > 500 * 1000) {
		debug("invalid bandwidth %u on downlink\n", downlink->bandwidth);
		return -1;
	}

	if (downlink->tx_power < 2 || downlink->tx_power > 17) {
		debug("invalid tx power %hhu on downlink\n", downlink->tx_power);
		return -1;
	}

	if (downlink->sf < 6 || downlink->sf > 12) {
		debug("invalid sf %hhu on downlink\n", downlink->sf);
		return -1;
	}

	return 0;
}

uint16_t downlink_insert(sqlite3 *database, downlink_t *downlink) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into downlink (id, kind, data, airtime, frequency, bandwidth, tx_power, sf, sent_at, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, downlink->kind);
	sqlite3_bind_blob(stmt, 2, downlink->data, downlink->data_len, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 3, (double)downlink->airtime / (16 * 1000));
	sqlite3_bind_int(stmt, 4, (int)downlink->frequency);
	sqlite3_bind_int(stmt, 5, (int)downlink->bandwidth);
	sqlite3_bind_int(stmt, 6, downlink->tx_power);
	sqlite3_bind_int(stmt, 7, downlink->sf);
	sqlite3_bind_int64(stmt, 8, downlink->sent_at);
	sqlite3_bind_blob(stmt, 9, downlink->device_id, sizeof(*downlink->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*downlink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*downlink->id));
			status = 500;
			goto cleanup;
		}
		memcpy(downlink->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("downlink from device %02x%02x is conflicting\n", (*downlink->device_id)[0], (*downlink->device_id)[1]);
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

void downlink_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	downlink_query_t query = {.limit = 16, .offset = 0};
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t downlinks_len;
	uint16_t status = downlink_select(database, bwt, &query, response, &downlinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found %hhu downlinks\n", downlinks_len);
	response->status = 200;
}

void downlink_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 14, &uuid_len);
	if (uuid_len != sizeof(*((downlink_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((downlink_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	downlink_t downlink = {.id = &id};
	uint16_t status = downlink_existing(database, bwt, &downlink);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = downlink_select_one(database, bwt, &downlink, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found downlink %02x%02x\n", *downlink.id[0], *downlink.id[1]);
	response->status = 200;
}

void downlink_create(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	downlink_t downlink = {.id = &id};
	if (request->body_len == 0 || downlink_parse(&downlink, request) == -1 || downlink_validate(&downlink) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = downlink_insert(database, &downlink);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("created downlink %02x%02x\n", (*downlink.id)[0], (*downlink.id)[1]);
	response->status = 201;
}
