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
#include "device.h"
#include "user-device.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

uint16_t downlink_select(octet_t *db, bwt_t *bwt, downlink_query_t *query, response_t *response, uint8_t *downlinks_len) {
	uint16_t status;

	uint8_t devices_len;
	user_t user = {.id = &bwt->id};
	status = user_device_select_by_user(db, &user, &devices_len);
	if (status != 0) {
		return status;
	}

	debug("select downlinks for user %02x%02x limit %hhu offset %u\n", bwt->id[0], bwt->id[1], query->limit, query->offset);

	char (*uuids)[32] = (char (*)[32])db->alpha;
	char (*files)[128] = (char (*)[128])db->bravo;
	off_t *offsets = (off_t *)db->charlie;
	time_t *sent_ats = (time_t *)db->delta;
	octet_stmt_t *stmts = (octet_stmt_t *)db->echo;
	uint8_t stmts_len = 0;

	for (uint8_t index = 0; index < devices_len; index++) {
		uint8_t (*device_id)[16] =
				(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);

		if (base16_encode(uuids[index], sizeof(uuids[index]), device_id, sizeof(*device_id)) == -1) {
			error("failed to encode uuid to base 16\n");
			status = 500;
			goto cleanup;
		}

		if (sprintf(files[index], "%s/%.*s/%s.data", db->directory, (int)sizeof(uuids[index]), uuids[index], downlink_file) == -1) {
			error("failed to sprintf uuid to file\n");
			status = 500;
			goto cleanup;
		}

		if (octet_open(&stmts[index], files[index], O_RDONLY, F_RDLCK) == -1) {
			status = octet_error();
			goto cleanup;
		}

		offsets[index] = stmts[index].stat.st_size - downlink_row.size;
		stmts_len += 1;
	}

	for (uint8_t index = 0; index < stmts_len; index++) {
		if (offsets[index] < 0) {
			continue;
		}
		if (octet_row_read(&stmts[index], files[index], offsets[index], &db->table[index * downlink_row.size], downlink_row.size) ==
				-1) {
			status = octet_error();
			goto cleanup;
		}
		sent_ats[index] = (time_t)octet_uint64_read(&db->table[index * downlink_row.size], downlink_row.sent_at);
	}

	while (*downlinks_len < query->limit) {
		int8_t index = -1;
		time_t last_sent_at = 0;
		for (int8_t ind = 0; ind < stmts_len; ind++) {
			if (offsets[ind] >= 0 && sent_ats[ind] > last_sent_at) {
				index = ind;
				last_sent_at = sent_ats[ind];
			}
		}

		if (index == -1) {
			status = 0;
			break;
		}

		if (query->offset == 0) {
			uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index * downlink_row.size], downlink_row.id);
			uint16_t frame = octet_uint16_read(&db->table[index * downlink_row.size], downlink_row.frame);
			uint8_t kind = octet_uint8_read(&db->table[index * downlink_row.size], downlink_row.kind);
			uint8_t data_len = octet_uint8_read(&db->table[index * downlink_row.size], downlink_row.data_len);
			uint8_t (*data)[32] = (uint8_t (*)[32])octet_blob_read(&db->table[index * downlink_row.size], downlink_row.data);
			uint16_t airtime = octet_uint16_read(&db->table[index * downlink_row.size], downlink_row.airtime);
			uint32_t frequency = octet_uint32_read(&db->table[index * downlink_row.size], downlink_row.frequency);
			uint32_t bandwidth = octet_uint32_read(&db->table[index * downlink_row.size], downlink_row.bandwidth);
			uint8_t sf = octet_uint8_read(&db->table[index * downlink_row.size], downlink_row.sf);
			uint8_t cr = octet_uint8_read(&db->table[index * downlink_row.size], downlink_row.cr);
			uint8_t tx_power = octet_uint8_read(&db->table[index * downlink_row.size], downlink_row.tx_power);
			uint8_t preamble_len = octet_uint8_read(&db->table[index * downlink_row.size], downlink_row.preamble_len);
			time_t sent_at = (time_t)octet_uint64_read(&db->table[index * downlink_row.size], downlink_row.sent_at);
			uint8_t (*device_id)[16] =
					(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);
			body_write(response, id, sizeof(*id));
			body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
			body_write(response, &kind, sizeof(kind));
			body_write(response, &data_len, sizeof(data_len));
			body_write(response, data, data_len);
			body_write(response, (uint16_t[]){hton16(airtime)}, sizeof(airtime));
			body_write(response, (uint32_t[]){hton32(frequency)}, sizeof(frequency));
			body_write(response, (uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
			body_write(response, &sf, sizeof(sf));
			body_write(response, &cr, sizeof(cr));
			body_write(response, &tx_power, sizeof(tx_power));
			body_write(response, &preamble_len, sizeof(preamble_len));
			body_write(response, (uint64_t[]){hton64((uint64_t)sent_at)}, sizeof(sent_at));
			body_write(response, device_id, sizeof(*device_id));
			*downlinks_len += 1;
		} else {
			query->offset -= 1;
		}
		offsets[index] -= downlink_row.size;

		if (octet_row_read(&stmts[index], files[index], offsets[index], &db->table[index * downlink_row.size], downlink_row.size) ==
				-1) {
			status = octet_error();
			goto cleanup;
		}
		sent_ats[index] = (time_t)octet_uint64_read(&db->table[index * downlink_row.size], downlink_row.sent_at);
	}

cleanup:
	for (uint8_t index = 0; index < stmts_len; index++) {
		octet_close(&stmts[index], files[index]);
	}
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
		uint16_t airtime = octet_uint16_read(db->row, downlink_row.airtime);
		uint32_t frequency = octet_uint32_read(db->row, downlink_row.frequency);
		uint32_t bandwidth = octet_uint32_read(db->row, downlink_row.bandwidth);
		uint8_t sf = octet_uint8_read(db->row, downlink_row.sf);
		uint8_t cr = octet_uint8_read(db->row, downlink_row.cr);
		uint8_t tx_power = octet_uint8_read(db->row, downlink_row.tx_power);
		uint8_t preamble_len = octet_uint8_read(db->row, downlink_row.preamble_len);
		time_t sent_at = (time_t)octet_uint64_read(db->row, downlink_row.sent_at);
		body_write(response, id, sizeof(*id));
		body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
		body_write(response, &kind, sizeof(kind));
		body_write(response, &data_len, sizeof(data_len));
		body_write(response, data, data_len);
		body_write(response, (uint16_t[]){hton16(airtime)}, sizeof(airtime));
		body_write(response, (uint32_t[]){hton32(frequency)}, sizeof(frequency));
		body_write(response, (uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
		body_write(response, &sf, sizeof(sf));
		body_write(response, &cr, sizeof(cr));
		body_write(response, &tx_power, sizeof(tx_power));
		body_write(response, &preamble_len, sizeof(preamble_len));
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
	if (downlink->data_len > 32) {
		debug("invalid data len %hhu on downlink\n", downlink->data_len);
		return -1;
	}

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

	uint8_t zone_id[16];
	device_t device = {.id = downlink->device_id, .zone_id = &zone_id};
	status = device_update_latest(db, &device, NULL, NULL, NULL, NULL, downlink);
	if (status != 0) {
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void downlink_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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
	uint16_t status = downlink_select(db, bwt, &query, response, &downlinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu downlinks\n", downlinks_len);
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
