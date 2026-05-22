#include "rule.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
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
#include <string.h>
#include <time.h>

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

int rule_parse(rule_t *rule, request_t *request) {
	request->body.pos = 0;

	if (request->body.len < request->body.pos + sizeof(rule->severity)) {
		debug("missing severity on rule\n");
		return -1;
	}
	rule->severity = (uint8_t)*body_read(request, sizeof(rule->severity));

	if (request->body.len < request->body.pos + sizeof(rule->field)) {
		debug("missing field on rule\n");
		return -1;
	}
	rule->field = (uint8_t)*body_read(request, sizeof(rule->field));

	if (request->body.len < request->body.pos + sizeof(rule->activate)) {
		debug("missing activate on rule\n");
		return -1;
	}
	memcpy(&rule->activate, body_read(request, sizeof(rule->activate)), sizeof(rule->activate));
	rule->activate = (int32_t)ntoh32((uint32_t)rule->activate);

	if (request->body.len < request->body.pos + sizeof(rule->disable)) {
		debug("missing disable on rule\n");
		return -1;
	}
	memcpy(&rule->disable, body_read(request, sizeof(rule->disable)), sizeof(rule->disable));
	rule->disable = (int32_t)ntoh32((uint32_t)rule->disable);

	if (rule->updated_at != NULL) {
		if (request->body.len < request->body.pos + sizeof(rule->created_at)) {
			debug("missing created at on rule\n");
			return -1;
		}
		memcpy(&rule->created_at, body_read(request, sizeof(rule->created_at)), sizeof(rule->created_at));
		rule->created_at = (time_t)ntoh64((uint64_t)rule->created_at);
	}

	if (request->body.len != request->body.pos) {
		debug("body len %u does not match body pos %u\n", request->body.len, request->body.pos);
		return -1;
	}

	return 0;
}

int rule_validate(rule_t *rule) {
	if (rule->severity > 2) {
		debug("invalid severity %hhu on rule\n", rule->severity);
		return -1;
	}

	if (rule->field > 5) {
		debug("invalid field %hhu on rule\n", rule->field);
		return -1;
	}

	return 0;
}

uint16_t rule_insert(octet_t *db, rule_t *rule) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), rule->device_id, sizeof(*rule->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, rule_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert rule for device %02x%02x created at %lu\n", (*rule->device_id)[0], (*rule->device_id)[1], rule->created_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - rule_row.size, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t created_at = (time_t)octet_uint64_read(db->row, rule_row.created_at);
		if (created_at <= rule->created_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= rule_row.size;
	}

	octet_uint8_write(db->row, rule_row.severity, rule->severity);
	octet_uint8_write(db->row, rule_row.field, rule->field);
	octet_int32_write(db->row, rule_row.activate, rule->activate);
	octet_int32_write(db->row, rule_row.disable, rule->disable);
	octet_uint64_write(db->row, rule_row.created_at, (uint64_t)rule->created_at);
	if (rule->updated_at != NULL) {
		octet_uint8_write(db->row, rule_row.updated_at_null, 0x01);
		octet_uint64_write(db->row, rule_row.updated_at, (uint64_t)*rule->updated_at);
	} else {
		octet_uint8_write(db->row, rule_row.updated_at_null, 0x00);
	}

	if (octet_row_write(&stmt, file, offset, db->row, rule_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t rule_update(octet_t *db, rule_t *rule) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), rule->device_id, sizeof(*rule->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, rule_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("update rule for device %02x%02x updated at %lu\n", (*rule->device_id)[0], (*rule->device_id)[1], *rule->updated_at);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("rule %lu for device %02x%02x not found\n", rule->created_at, (*rule->device_id)[0], (*rule->device_id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t created_at = (time_t)octet_uint64_read(db->row, rule_row.created_at);
		if (created_at == rule->created_at) {
			octet_uint8_write(db->row, rule_row.severity, rule->severity);
			octet_uint8_write(db->row, rule_row.field, rule->field);
			octet_int32_write(db->row, rule_row.activate, rule->activate);
			octet_int32_write(db->row, rule_row.disable, rule->disable);
			octet_uint8_write(db->row, rule_row.updated_at_null, 0x01);
			octet_uint64_write(db->row, rule_row.updated_at, (uint64_t)*rule->updated_at);
			if (octet_row_write(&stmt, file, offset, db->row, rule_row.size) == -1) {
				status = octet_error();
				goto cleanup;
			}
			status = 0;
			break;
		}
		offset += rule_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t rule_delete(octet_t *db, rule_t *rule) {
	uint16_t status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), rule->device_id, sizeof(*rule->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, rule_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("delete rule for device %02x%02x created at %lu\n", (*rule->device_id)[0], (*rule->device_id)[1], rule->created_at);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("rule %lu for device %02x%02x not found\n", rule->created_at, (*rule->device_id)[0], (*rule->device_id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t created_at = (time_t)octet_uint64_read(db->row, rule_row.created_at);
		if (created_at == rule->created_at) {
			break;
		}
		offset += rule_row.size;
	}

	off_t index = offset + rule_row.size;
	while (index < stmt.stat.st_size) {
		if (octet_row_read(&stmt, file, index, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		if (octet_row_write(&stmt, file, index - rule_row.size, db->row, rule_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		index += rule_row.size;
	}

	if (octet_trunc(&stmt, file, stmt.stat.st_size - rule_row.size)) {
		status = 500;
		goto cleanup;
	}

	status = 0;

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

void rule_create(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

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

	rule_t rule = {.created_at = time(NULL), .updated_at = NULL, .device_id = device.id};
	if (request->body.len == 0 || rule_parse(&rule, request) == -1 || rule_validate(&rule) == -1) {
		response->status = 400;
		return;
	}

	status = rule_insert(db, &rule);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("created rule for device %02x%02x\n", (*rule.device_id)[0], (*rule.device_id)[1]);
	response->status = 201;
}

void rule_modify(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

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

	rule_t rule = {.updated_at = (time_t[]){time(NULL)}, .device_id = device.id};
	if (request->body.len == 0 || rule_parse(&rule, request) == -1 || rule_validate(&rule) == -1) {
		response->status = 400;
		return;
	}

	status = rule_update(db, &rule);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("updated rule for device %02x%02x\n", (*rule.device_id)[0], (*rule.device_id)[1]);
	response->status = 200;
}

void rule_remove(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

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

	rule_t rule = {.device_id = device.id};
	if (request->body.len < request->body.pos + sizeof(rule.created_at)) {
		debug("missing created at on rule\n");
		response->status = 400;
		return;
	}
	memcpy(&rule.created_at, body_read(request, sizeof(rule.created_at)), sizeof(rule.created_at));
	rule.created_at = (time_t)ntoh64((uint64_t)rule.created_at);
	status = rule_delete(db, &rule);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("deleted rule for device %02x%02x\n", (*rule.device_id)[0], (*rule.device_id)[1]);
	response->status = 200;
}
