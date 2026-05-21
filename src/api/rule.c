#include "rule.h"
#include "../lib/base16.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/response.h"
#include "cache.h"
#include "device.h"
#include "user-device.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>

const char *rule_file = "rule";

const rule_row_t rule_row = {
		.severity = 0,
		.field = 1,
		.activate = 2,
		.disable = 6,
		.created_at = 10,
		.updated_at_null = 18,
		.updated_at = 19,
		.size = 27,
};

uint16_t rule_select_by_device(octet_t *db, device_t *device, rule_query_t *query, response_t *response, uint8_t *rules_len) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, rule_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select rules for device %02x%02x limit %hhu offset %u\n", (*device->id)[0], (*device->id)[1], query->limit,
				query->offset);

	off_t offset = stmt.stat.st_size - rule_row.size - query->offset * rule_row.size;
	while (true) {
		if (offset < 0 || *rules_len >= query->limit) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t severity = octet_uint8_read(db->row, rule_row.severity);
		uint8_t field = octet_uint8_read(db->row, rule_row.field);
		int32_t activate = octet_int32_read(db->row, rule_row.activate);
		int32_t disable = octet_int32_read(db->row, rule_row.disable);
		uint64_t created_at = octet_uint64_read(db->row, rule_row.created_at);
		uint8_t updated_at_null = octet_uint8_read(db->row, rule_row.updated_at_null);
		uint64_t updated_at = octet_uint64_read(db->row, rule_row.updated_at);
		body_write(response, &severity, sizeof(severity));
		body_write(response, &field, sizeof(field));
		body_write(response, (uint32_t[]){hton32((uint32_t)activate)}, sizeof(activate));
		body_write(response, (uint32_t[]){hton32((uint32_t)disable)}, sizeof(disable));
		body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
		body_write(response, (uint8_t[]){updated_at_null != 0x00}, sizeof(updated_at_null));
		if (updated_at_null != 0x00) {
			body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
		}
		*rules_len += 1;
		offset -= rule_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

void rule_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	rule_query_t query = {.limit = 0, .offset = 0};
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

	uint8_t rules_len = 0;
	status = rule_select_by_device(db, &device, &query, response, &rules_len);
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
	info("found %hu rules\n", rules_len);
	response->status = 200;
}
