#include "downlink.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>
#include <stdint.h>
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
