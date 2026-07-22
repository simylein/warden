#include "alert.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/config.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "cache.h"
#include "device.h"
#include "user-device.h"
#include "user.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

const char *alert_file = "alert";

const alert_row_t alert_row = {
		.severity = 0,
		.field = 1,
		.edge = 2,
		.value = 3,
		.issued_at = 7,
		.resolved_at_null = 15,
		.resolved_at = 16,
		.size = 24,
};

uint16_t alert_select(octet_t *db, bwt_t *bwt, alert_query_t *query, response_t *response, uint8_t *alerts_len) {
	uint16_t status;

	uint8_t devices_len;
	user_t user = {.id = &bwt->id};
	status = user_device_select_by_user(db, &user, &devices_len);
	if (status != 0) {
		return status;
	}

	debug("select alerts for user %02x%02x limit %hhu offset %u\n", bwt->id[0], bwt->id[1], query->limit, query->offset);

	char (*uuids)[16] = (char (*)[16])db->alpha;
	char (*files)[128] = (char (*)[128])db->bravo;
	off_t *offsets = (off_t *)db->charlie;
	time_t *issued_ats = (time_t *)db->delta;
	octet_stmt_t *stmts = (octet_stmt_t *)db->echo;
	uint8_t stmts_len = 0;
	for (uint8_t index = 0; index < devices_len; index++) {
		uint8_t (*device_id)[8] =
				(uint8_t (*)[8])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);

		if (base16_encode(uuids[index], sizeof(uuids[index]), device_id, sizeof(*device_id)) == -1) {
			error("failed to encode uuid to base 16\n");
			status = 500;
			goto cleanup;
		}

		if (sprintf(files[index], "%s/%.*s/%s.data", db->directory, (int)sizeof(uuids[index]), uuids[index], alert_file) == -1) {
			error("failed to sprintf uuid to file\n");
			status = 500;
			goto cleanup;
		}

		if (octet_open(&stmts[index], files[index], O_RDONLY, F_RDLCK) == -1) {
			status = octet_error();
			goto cleanup;
		}

		offsets[index] = stmts[index].stat.st_size - alert_row.size;
		stmts_len += 1;
	}

	for (uint8_t index = 0; index < stmts_len; index++) {
		if (offsets[index] < 0) {
			continue;
		}
		if (octet_row_read(&stmts[index], files[index], offsets[index], &db->table[index * alert_row.size], alert_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		issued_ats[index] = (time_t)octet_uint64_read(&db->table[index * alert_row.size], alert_row.issued_at);
	}

	while (*alerts_len < query->limit) {
		int8_t index = -1;
		time_t last_issued_at = 0;
		for (int8_t ind = 0; ind < stmts_len; ind++) {
			if (offsets[ind] >= 0 && issued_ats[ind] > last_issued_at) {
				index = ind;
				last_issued_at = issued_ats[ind];
			}
		}

		if (index == -1) {
			status = 0;
			break;
		}

		if (query->offset == 0) {
			uint8_t severity = octet_uint8_read(&db->table[index * alert_row.size], alert_row.severity);
			uint8_t field = octet_uint8_read(&db->table[index * alert_row.size], alert_row.field);
			uint8_t edge = octet_uint8_read(&db->table[index * alert_row.size], alert_row.edge);
			int32_t value = octet_int32_read(&db->table[index * alert_row.size], alert_row.value);
			time_t issued_at = (time_t)octet_uint64_read(&db->table[index * alert_row.size], alert_row.issued_at);
			uint8_t resolved_at_null = octet_uint8_read(&db->table[index * alert_row.size], alert_row.resolved_at_null);
			time_t resolved_at = (time_t)octet_uint64_read(&db->table[index * alert_row.size], alert_row.resolved_at);
			uint8_t (*device_id)[8] =
					(uint8_t (*)[8])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);
			body_write(response, &severity, sizeof(severity));
			body_write(response, &field, sizeof(field));
			body_write(response, &edge, sizeof(edge));
			body_write(response, (uint32_t[]){hton32((uint32_t)value)}, sizeof(value));
			body_write(response, (uint64_t[]){hton64((uint64_t)issued_at)}, sizeof(issued_at));
			body_write(response, (uint8_t[]){resolved_at_null != 0x00}, sizeof(resolved_at_null));
			if (resolved_at_null != 0x00) {
				body_write(response, (uint64_t[]){hton64((uint64_t)resolved_at)}, sizeof(resolved_at));
			}
			body_write(response, device_id, sizeof(*device_id));
			*alerts_len += 1;
		} else {
			query->offset -= 1;
		}
		offsets[index] -= alert_row.size;
		if (offsets[index] < 0) {
			continue;
		}

		if (octet_row_read(&stmts[index], files[index], offsets[index], &db->table[index * alert_row.size], alert_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		issued_ats[index] = (time_t)octet_uint64_read(&db->table[index * alert_row.size], alert_row.issued_at);
	}

cleanup:
	for (uint8_t index = 0; index < stmts_len; index++) {
		octet_close(&stmts[index], files[index]);
	}
	return status;
}

uint16_t alert_select_by_device(octet_t *db, device_t *device, alert_query_t *query, response_t *response,
																uint8_t *alerts_len) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, alert_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select alerts for device %02x%02x limit %hhu offset %u\n", (*device->id)[0], (*device->id)[1], query->limit,
				query->offset);

	off_t offset = stmt.stat.st_size - alert_row.size - query->offset * alert_row.size;
	while (true) {
		if (offset < 0 || *alerts_len >= query->limit) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, alert_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t severity = octet_uint8_read(db->row, alert_row.severity);
		uint8_t field = octet_uint8_read(db->row, alert_row.field);
		uint8_t edge = octet_uint8_read(db->row, alert_row.edge);
		int32_t value = octet_int32_read(db->row, alert_row.value);
		time_t issued_at = (time_t)octet_uint64_read(db->row, alert_row.issued_at);
		uint8_t resolved_at_null = octet_uint8_read(db->row, alert_row.resolved_at_null);
		time_t resolved_at = (time_t)octet_uint64_read(db->row, alert_row.resolved_at);
		body_write(response, &severity, sizeof(severity));
		body_write(response, &field, sizeof(field));
		body_write(response, &edge, sizeof(edge));
		body_write(response, (uint32_t[]){hton32((uint32_t)value)}, sizeof(value));
		body_write(response, (uint64_t[]){hton64((uint64_t)issued_at)}, sizeof(issued_at));
		body_write(response, (uint8_t[]){resolved_at_null != 0x00}, sizeof(resolved_at_null));
		if (resolved_at_null != 0x00) {
			body_write(response, (uint64_t[]){hton64((uint64_t)resolved_at)}, sizeof(resolved_at));
		}
		*alerts_len += 1;
		offset -= alert_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t alert_insert(octet_t *db, alert_t *alert) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), alert->device_id, sizeof(*alert->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, alert_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert alert for device %02x%02x issued at %lu\n", (*alert->device_id)[0], (*alert->device_id)[1], alert->issued_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - alert_row.size, db->row, alert_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t issued_at = (time_t)octet_uint64_read(db->row, alert_row.issued_at);
		if (issued_at <= alert->issued_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, alert_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= alert_row.size;
	}

	octet_uint8_write(db->row, alert_row.severity, alert->severity);
	octet_uint8_write(db->row, alert_row.field, alert->field);
	octet_uint8_write(db->row, alert_row.edge, alert->edge);
	octet_int32_write(db->row, alert_row.value, alert->value);
	octet_uint64_write(db->row, alert_row.issued_at, (uint64_t)alert->issued_at);
	if (alert->resolved_at != NULL) {
		octet_uint8_write(db->row, alert_row.resolved_at_null, 0x01);
		octet_uint64_write(db->row, alert_row.resolved_at, (uint64_t)*alert->resolved_at);
	} else {
		octet_uint8_write(db->row, alert_row.resolved_at_null, 0x00);
	}

	if (octet_row_write(&stmt, file, offset, db->row, alert_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t alert_update(octet_t *db, alert_t *alert) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), alert->device_id, sizeof(*alert->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, alert_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("update alert for device %02x%02x resolved at %lu\n", (*alert->device_id)[0], (*alert->device_id)[1],
				*alert->resolved_at);

	off_t offset = stmt.stat.st_size - alert_row.size;
	while (true) {
		if (offset < 0) {
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, alert_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t severity = octet_uint8_read(db->row, alert_row.severity);
		uint8_t field = octet_uint8_read(db->row, alert_row.field);
		uint8_t edge = octet_uint8_read(db->row, alert_row.edge);
		time_t issued_at = (time_t)octet_uint64_read(db->row, alert_row.issued_at);
		uint8_t resolved_at_null = octet_uint8_read(db->row, alert_row.resolved_at_null);
		if (issued_at + alert_lookback < *alert->resolved_at) {
			status = 0;
			break;
		}
		if (severity == alert->severity && field == alert->field && edge == alert->edge && resolved_at_null == 0x00) {
			octet_uint8_write(db->row, alert_row.resolved_at_null, 0x01);
			octet_uint64_write(db->row, alert_row.resolved_at, (uint64_t)*alert->resolved_at);
			if (octet_row_write(&stmt, file, offset, db->row, alert_row.size) == -1) {
				status = octet_error();
				goto cleanup;
			}
			status = 0;
			break;
		}
		offset -= alert_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

void alert_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	alert_query_t query = {.limit = 0, .offset = 0};
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

	uint8_t alerts_len = 0;
	uint16_t status = alert_select(db, bwt, &query, response, &alerts_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu alerts\n", alerts_len);
	response->status = 200;
}

void alert_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 12, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[8];
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

	alert_query_t query = {.limit = 0, .offset = 0};
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

	uint8_t alerts_len = 0;
	status = alert_select_by_device(db, &device, &query, response, &alerts_len);
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
	info("found %hu alerts\n", alerts_len);
	response->status = 200;
}
