#include "downlink.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "cache.h"
#include "database.h"
#include "device.h"
#include "user-device.h"
#include <fcntl.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *downlink_table = "downlink";
const char *downlink_schema = "create table downlink ("
															"id blob primary key, "
															"frame integer not null, "
															"kind integer not null, "
															"data blob not null, "
															"airtime real not null, "
															"frequency integer not null, "
															"bandwidth integer not null, "
															"sf integer not null, "
															"cr integer not null, "
															"tx_power integer not null, "
															"preamble_len integer not null, "
															"sent_at timestamp not null, "
															"device_id blob not null, "
															"foreign key (device_id) references device(id) on delete cascade"
															")";

const char *downlink_file = "downlink";

const downlink_row_t downlink_row = {
		.id = 0,
		.frame = 16,
		.kind = 18,
		.data_len = 19,
		.data = 20,
		.airtime = 52,
		.frequency = 54,
		.bandwidth = 58,
		.sf = 62,
		.cr = 63,
		.tx_power = 64,
		.preamble_len = 65,
		.sent_at = 66,
		.size = 74,
};

uint16_t downlink_existing(sqlite3 *database, bwt_t *bwt, downlink_t *downlink) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"downlink.id, "
										"user_device.device_id "
										"from downlink "
										"left join user_device on user_device.device_id = downlink.device_id and user_device.user_id = ? "
										"where downlink.id = ?";
	debug("select existing downlink %02x%02x for user %02x%02x\n", (*downlink->id)[0], (*downlink->id)[1], bwt->id[0],
				bwt->id[1]);

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
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t downlink_select(sqlite3 *database, bwt_t *bwt, downlink_query_t *query, response_t *response, uint8_t *downlinks_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, frame, kind, data, sf, tx_power, sent_at, downlink.device_id from downlink "
										"join user_device on user_device.device_id = downlink.device_id and user_device.user_id = ? "
										"order by sent_at desc "
										"limit ? offset ?";
	debug("select downlinks for user %02x%02x limit %hhu offset %u\n", bwt->id[0], bwt->id[1], query->limit, query->offset);

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
			const uint16_t frame = (uint16_t)sqlite3_column_int(stmt, 1);
			const uint8_t kind = (uint8_t)sqlite3_column_int(stmt, 2);
			const uint8_t *data = sqlite3_column_blob(stmt, 3);
			const size_t data_len = (size_t)sqlite3_column_bytes(stmt, 3);
			if (data_len > UINT8_MAX) {
				error("data length %zu exceeds buffer length %hhu\n", data_len, UINT8_MAX);
				status = 500;
				goto cleanup;
			}
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 4);
			const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 5);
			const time_t sent_at = (time_t)sqlite3_column_int64(stmt, 6);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 7);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
			if (device_id_len != sizeof(*((downlink_t *)0)->device_id)) {
				error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((downlink_t *)0)->device_id));
				status = 500;
				goto cleanup;
			}
			body_write(response, id, id_len);
			body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
			body_write(response, &kind, sizeof(kind));
			body_write(response, &data_len, sizeof(uint8_t));
			body_write(response, data, data_len);
			body_write(response, &sf, sizeof(sf));
			body_write(response, &tx_power, sizeof(tx_power));
			body_write(response, (uint64_t[]){hton64((uint64_t)sent_at)}, sizeof(sent_at));
			body_write(response, device_id, device_id_len);
			*downlinks_len += 1;
		} else if (result == SQLITE_DONE) {
			status = 0;
			break;
		} else {
			status = database_error(database, result);
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
										"downlink.id, downlink.frame, downlink.kind, downlink.data, "
										"downlink.airtime, downlink.frequency, downlink.bandwidth, "
										"downlink.sf, downlink.cr, downlink.tx_power, downlink.preamble_len, "
										"downlink.sent_at, downlink.device_id "
										"from downlink "
										"join user_device on user_device.device_id = downlink.device_id and user_device.user_id = ? "
										"where downlink.id = ?";
	debug("select downlink %02x%02x for user %02x%02x\n", (*downlink->id)[0], (*downlink->id)[1], bwt->id[0], bwt->id[1]);

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
		const uint16_t frame = (uint16_t)sqlite3_column_int(stmt, 1);
		const uint8_t kind = (uint8_t)sqlite3_column_int(stmt, 2);
		const uint8_t *data = sqlite3_column_blob(stmt, 3);
		const size_t data_len = (size_t)sqlite3_column_bytes(stmt, 3);
		if (data_len > UINT8_MAX) {
			error("data length %zu exceeds buffer length %hhu\n", data_len, UINT8_MAX);
			status = 500;
			goto cleanup;
		}
		const double airtime = sqlite3_column_double(stmt, 4);
		const uint32_t frequency = (uint32_t)sqlite3_column_int(stmt, 5);
		const uint32_t bandwidth = (uint32_t)sqlite3_column_int(stmt, 6);
		const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 7);
		const uint8_t cr = (uint8_t)sqlite3_column_int(stmt, 8);
		const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 9);
		const uint8_t preamble_len = (uint8_t)sqlite3_column_int(stmt, 10);
		const time_t sent_at = (time_t)sqlite3_column_int64(stmt, 11);
		const uint8_t *device_id = sqlite3_column_blob(stmt, 12);
		const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 12);
		if (device_id_len != sizeof(*((downlink_t *)0)->device_id)) {
			error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((downlink_t *)0)->device_id));
			status = 500;
			goto cleanup;
		}
		body_write(response, id, id_len);
		body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
		body_write(response, &kind, sizeof(kind));
		body_write(response, &data_len, sizeof(uint8_t));
		body_write(response, data, data_len);
		body_write(response, (uint16_t[]){hton16((uint16_t)(airtime * 16 * 1000))}, sizeof(uint16_t));
		body_write(response, (uint32_t[]){hton32(frequency)}, sizeof(frequency));
		body_write(response, (uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
		body_write(response, &sf, sizeof(sf));
		body_write(response, &cr, sizeof(cr));
		body_write(response, &tx_power, sizeof(tx_power));
		body_write(response, &preamble_len, sizeof(preamble_len));
		body_write(response, (uint64_t[]){hton64((uint64_t)sent_at)}, sizeof(sent_at));
		body_write(response, device_id, device_id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("downlink %02x%02x not found\n", (*downlink->id)[0], (*downlink->id)[1]);
		status = 404;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t downlink_select_by_device(octet_t *db, device_t *device, downlink_query_t *query, response_t *response,
																	 uint8_t *downlinks_len) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, downlink_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select downlinks for device %02x%02x limit %hhu offset %u\n", (*device->id)[0], (*device->id)[1], query->limit,
				query->offset);

	off_t offset = stmt.stat.st_size - downlink_row.size - query->offset * downlink_row.size;
	while (true) {
		if (offset < 0 || *downlinks_len >= query->limit) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, downlink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, downlink_row.id);
		uint16_t frame = octet_uint16_read(db->row, downlink_row.frame);
		uint8_t kind = octet_uint8_read(db->row, downlink_row.kind);
		uint8_t data_len = octet_uint8_read(db->row, downlink_row.data_len);
		uint8_t (*data)[32] = (uint8_t (*)[32])octet_blob_read(db->row, downlink_row.data);
		uint8_t sf = octet_uint8_read(db->row, downlink_row.sf);
		uint8_t tx_power = octet_uint8_read(db->row, downlink_row.tx_power);
		time_t sent_at = (time_t)octet_uint64_read(db->row, downlink_row.sent_at);
		body_write(response, id, sizeof(*id));
		body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
		body_write(response, &kind, sizeof(kind));
		body_write(response, &data_len, sizeof(data_len));
		body_write(response, data, data_len);
		body_write(response, &sf, sizeof(sf));
		body_write(response, &tx_power, sizeof(tx_power));
		body_write(response, (uint64_t[]){hton64((uint64_t)sent_at)}, sizeof(sent_at));
		*downlinks_len += 1;
		offset -= downlink_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

int downlink_parse(downlink_t *downlink, request_t *request) {
	request->body.pos = 0;

	if (request->body.len < request->body.pos + sizeof(downlink->frame)) {
		debug("missing frame on downlink\n");
		return -1;
	}
	memcpy(&downlink->frame, body_read(request, sizeof(downlink->frame)), sizeof(downlink->frame));
	downlink->frame = ntoh16(downlink->frame);

	if (request->body.len < request->body.pos + sizeof(downlink->kind)) {
		debug("missing kind on downlink\n");
		return -1;
	}
	downlink->kind = *(uint8_t *)body_read(request, sizeof(downlink->kind));

	if (request->body.len < request->body.pos + sizeof(downlink->data_len)) {
		debug("missing data len on downlink\n");
		return -1;
	}
	downlink->data_len = *(uint8_t *)body_read(request, sizeof(downlink->data_len));

	if (request->body.len < request->body.pos + downlink->data_len) {
		debug("missing data on downlink\n");
		return -1;
	}
	downlink->data = (uint8_t *)body_read(request, downlink->data_len);

	if (request->body.len < request->body.pos + sizeof(downlink->airtime)) {
		debug("missing airtime on downlink\n");
		return -1;
	}
	memcpy(&downlink->airtime, body_read(request, sizeof(downlink->airtime)), sizeof(downlink->airtime));
	downlink->airtime = ntoh16(downlink->airtime);

	if (request->body.len < request->body.pos + sizeof(downlink->frequency)) {
		debug("missing frequency on downlink\n");
		return -1;
	}
	memcpy(&downlink->frequency, body_read(request, sizeof(downlink->frequency)), sizeof(downlink->frequency));
	downlink->frequency = ntoh32(downlink->frequency);

	if (request->body.len < request->body.pos + sizeof(downlink->bandwidth)) {
		debug("missing bandwidth on downlink\n");
		return -1;
	}
	memcpy(&downlink->bandwidth, body_read(request, sizeof(downlink->bandwidth)), sizeof(downlink->bandwidth));
	downlink->bandwidth = ntoh32(downlink->bandwidth);

	if (request->body.len < request->body.pos + sizeof(downlink->sf)) {
		debug("missing sf on downlink\n");
		return -1;
	}
	downlink->sf = *(uint8_t *)body_read(request, sizeof(downlink->sf));

	if (request->body.len < request->body.pos + sizeof(downlink->cr)) {
		debug("missing cr on downlink\n");
		return -1;
	}
	downlink->cr = *(uint8_t *)body_read(request, sizeof(downlink->cr));

	if (request->body.len < request->body.pos + sizeof(downlink->tx_power)) {
		debug("missing tx power on downlink\n");
		return -1;
	}
	downlink->tx_power = *(uint8_t *)body_read(request, sizeof(downlink->tx_power));

	if (request->body.len < request->body.pos + sizeof(downlink->preamble_len)) {
		debug("missing preamble len on downlink\n");
		return -1;
	}
	downlink->preamble_len = *(uint8_t *)body_read(request, sizeof(downlink->preamble_len));

	if (request->body.len < request->body.pos + sizeof(downlink->sent_at)) {
		debug("missing sent at on downlink\n");
		return -1;
	}
	memcpy(&downlink->sent_at, body_read(request, sizeof(downlink->sent_at)), sizeof(downlink->sent_at));
	downlink->sent_at = (time_t)ntoh64((uint64_t)downlink->sent_at);

	if (request->body.len < request->body.pos + sizeof(*downlink->device_id)) {
		debug("missing device id on downlink\n");
		return -1;
	}
	downlink->device_id = (uint8_t (*)[16])body_read(request, sizeof(*downlink->device_id));

	if (request->body.len != request->body.pos) {
		debug("body len %u does not match body pos %u\n", request->body.len, request->body.pos);
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

	if (downlink->sf < 6 || downlink->sf > 12) {
		debug("invalid sf %hhu on downlink\n", downlink->sf);
		return -1;
	}

	if (downlink->cr < 5 || downlink->cr > 8) {
		debug("invalid cr %hhu on downlink\n", downlink->cr);
		return -1;
	}

	if (downlink->tx_power < 2 || downlink->tx_power > 17) {
		debug("invalid tx power %hhu on downlink\n", downlink->tx_power);
		return -1;
	}

	if (downlink->preamble_len < 6 || downlink->preamble_len > 21) {
		debug("invalid preamble len %hhu on downlink\n", downlink->preamble_len);
		return -1;
	}

	return 0;
}

uint16_t downlink_insert(octet_t *db, downlink_t *downlink) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*downlink->id); index++) {
		(*downlink->id)[index] = (uint8_t)(rand() & 0xff);
	}

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), downlink->device_id, sizeof(*downlink->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, downlink_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert downlink for device %02x%02x sent at %lu\n", (*downlink->device_id)[0], (*downlink->device_id)[1],
				downlink->sent_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - downlink_row.size, db->row, downlink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t sent_at = (time_t)octet_uint64_read(db->row, downlink_row.sent_at);
		if (sent_at <= downlink->sent_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, downlink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= downlink_row.size;
	}

	octet_blob_write(db->row, downlink_row.id, (uint8_t *)downlink->id, sizeof(*downlink->id));
	octet_uint16_write(db->row, downlink_row.frame, downlink->frame);
	octet_uint8_write(db->row, downlink_row.kind, downlink->kind);
	octet_uint8_write(db->row, downlink_row.data_len, downlink->data_len);
	octet_blob_write(db->row, downlink_row.data, downlink->data, downlink->data_len);
	octet_uint16_write(db->row, downlink_row.airtime, downlink->airtime);
	octet_uint32_write(db->row, downlink_row.frequency, downlink->frequency);
	octet_uint32_write(db->row, downlink_row.bandwidth, downlink->bandwidth);
	octet_uint8_write(db->row, downlink_row.sf, downlink->sf);
	octet_uint8_write(db->row, downlink_row.cr, downlink->cr);
	octet_uint8_write(db->row, downlink_row.tx_power, downlink->tx_power);
	octet_uint8_write(db->row, downlink_row.preamble_len, downlink->preamble_len);
	octet_uint64_write(db->row, downlink_row.sent_at, (uint64_t)downlink->sent_at);

	if (octet_row_write(&stmt, file, offset, db->row, downlink_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void downlink_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	const char *limit;
	size_t limit_len;
	if (strnfind(request->search.ptr, request->search.len, "limit=", "&", &limit, &limit_len, 4) == -1) {
		response->status = 400;
		return;
	}

	const char *offset;
	size_t offset_len;
	if (strnfind(request->search.ptr, request->search.len, "offset=", "", &offset, &offset_len, 4) == -1) {
		response->status = 400;
		return;
	}

	downlink_query_t query = {.limit = 0, .offset = 0};
	if (strnto8(limit, limit_len, &query.limit) == -1 || strnto32(offset, offset_len, &query.offset) == -1) {
		warn("failed to parse query limit %*.s offset %*.s\n", (int)limit_len, limit, (int)offset_len, offset);
		response->status = 400;
		return;
	}

	if (query.limit < 4 || query.limit > 64 || query.offset > 16777216) {
		warn("failed to validate query limit %hhu offset %u\n", query.limit, query.offset);
		response->status = 400;
		return;
	}

	uint8_t downlinks_len = 0;
	uint16_t status = downlink_select(database, bwt, &query, response, &downlinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu downlinks\n", downlinks_len);
	response->status = 200;
}

void downlink_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 14, &uuid_len);
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

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found downlink %02x%02x\n", (*downlink.id)[0], (*downlink.id)[1]);
	response->status = 200;
}

void downlink_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 12, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	const char *limit;
	size_t limit_len;
	if (strnfind(request->search.ptr, request->search.len, "limit=", "&", &limit, &limit_len, 4) == -1) {
		response->status = 400;
		return;
	}

	const char *offset;
	size_t offset_len;
	if (strnfind(request->search.ptr, request->search.len, "offset=", "", &offset, &offset_len, 4) == -1) {
		response->status = 400;
		return;
	}

	downlink_query_t query = {.limit = 0, .offset = 0};
	if (strnto8(limit, limit_len, &query.limit) == -1 || strnto32(offset, offset_len, &query.offset) == -1) {
		warn("failed to parse query limit %*.s offset %*.s\n", (int)limit_len, limit, (int)offset_len, offset);
		response->status = 400;
		return;
	}

	if (query.limit < 4 || query.limit > 64 || query.offset > 16777216) {
		warn("failed to validate query limit %hhu offset %u\n", query.limit, query.offset);
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(db, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	user_device_t user_device = {.user_id = &bwt->id, .device_id = device.id};
	status = user_device_existing(db, &user_device);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint8_t downlinks_len = 0;
	status = downlink_select_by_device(db, &device, &query, response, &downlinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	cache_device_t cache_device;
	if (cache_device_read(&cache_device, &device) != -1) {
		header_write(response, "device-name:%.*s\r\n", cache_device.name_len, cache_device.name);
		if (cache_device.zone_name_len != 0) {
			header_write(response, "device-zone-name:%.*s\r\n", cache_device.zone_name_len, cache_device.zone_name);
		}
	}
	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hu downlinks\n", downlinks_len);
	response->status = 200;
}

void downlink_create(octet_t *db, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	downlink_t downlink = {.id = &id};
	if (request->body.len == 0 || downlink_parse(&downlink, request) == -1 || downlink_validate(&downlink) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = downlink_insert(db, &downlink);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("created downlink %02x%02x\n", (*downlink.id)[0], (*downlink.id)[1]);
	response->status = 201;
}
