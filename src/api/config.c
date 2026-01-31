#include "config.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "device.h"
#include "user-device.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *config_file = "config";

const config_row_t config_row = {
		.id = 0,
		.led_debug = 16,
		.reading_enable = 17,
		.metric_enable = 18,
		.buffer_enable = 19,
		.reading_interval = 20,
		.metric_interval = 22,
		.buffer_interval = 24,
		.captured_at = 26,
		.uplink_id = 34,
		.size = 50,
};

uint16_t config_select_one_by_device(octet_t *db, device_t *device, response_t *response) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, config_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select config for device %02x%02x\n", (*device->id)[0], (*device->id)[1]);

	off_t offset = stmt.stat.st_size - config_row.size;
	if (offset < 0) {
		debug("no config for device %02x%02x found\n", (*device->id)[0], (*device->id)[1]);
		status = 204;
		goto cleanup;
	}

	if (octet_row_read(&stmt, file, offset, db->row, config_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	bool led_debug = octet_uint8_read(db->row, config_row.led_debug);
	bool reading_enable = octet_uint8_read(db->row, config_row.reading_enable);
	bool metric_enable = octet_uint8_read(db->row, config_row.metric_enable);
	bool buffer_enable = octet_uint8_read(db->row, config_row.buffer_enable);
	uint16_t reading_interval = octet_uint16_read(db->row, config_row.reading_interval);
	uint16_t metric_interval = octet_uint16_read(db->row, config_row.metric_interval);
	uint16_t buffer_interval = octet_uint16_read(db->row, config_row.buffer_interval);
	time_t captured_at = (time_t)octet_uint64_read(db->row, config_row.captured_at);
	uint8_t (*uplink_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, config_row.uplink_id);
	body_write(response, &led_debug, sizeof(led_debug));
	body_write(response, &reading_enable, sizeof(reading_enable));
	body_write(response, &metric_enable, sizeof(metric_enable));
	body_write(response, &buffer_enable, sizeof(buffer_enable));
	body_write(response, (uint16_t[]){hton16((uint16_t)reading_interval)}, sizeof(reading_interval));
	body_write(response, (uint16_t[]){hton16((uint16_t)metric_interval)}, sizeof(metric_interval));
	body_write(response, (uint16_t[]){hton16((uint16_t)buffer_interval)}, sizeof(buffer_interval));
	body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
	body_write(response, uplink_id, sizeof(*uplink_id));

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

int config_parse(config_t *config, request_t *request) {
	request->body.pos = 0;

	if (request->body.len < request->body.pos + sizeof(config->led_debug)) {
		debug("missing led debug on config\n");
		return -1;
	}
	config->led_debug = *body_read(request, sizeof(config->led_debug));

	if (request->body.len < request->body.pos + sizeof(config->reading_enable)) {
		debug("missing reading enable on config\n");
		return -1;
	}
	config->reading_enable = *body_read(request, sizeof(config->reading_enable));

	if (request->body.len < request->body.pos + sizeof(config->metric_enable)) {
		debug("missing metric enable on config\n");
		return -1;
	}
	config->metric_enable = *body_read(request, sizeof(config->metric_enable));

	if (request->body.len < request->body.pos + sizeof(config->buffer_enable)) {
		debug("missing buffer enable on config\n");
		return -1;
	}
	config->buffer_enable = *body_read(request, sizeof(config->buffer_enable));

	if (request->body.len < request->body.pos + sizeof(config->reading_interval)) {
		debug("missing reading interval on config\n");
		return -1;
	}
	memcpy(&config->reading_interval, body_read(request, sizeof(config->reading_interval)), sizeof(config->reading_interval));
	config->reading_interval = ntoh16(config->reading_interval);

	if (request->body.len < request->body.pos + sizeof(config->metric_interval)) {
		debug("missing metric interval on config\n");
		return -1;
	}
	memcpy(&config->metric_interval, body_read(request, sizeof(config->metric_interval)), sizeof(config->metric_interval));
	config->metric_interval = ntoh16(config->metric_interval);

	if (request->body.len < request->body.pos + sizeof(config->buffer_interval)) {
		debug("missing buffer interval on config\n");
		return -1;
	}
	memcpy(&config->buffer_interval, body_read(request, sizeof(config->buffer_interval)), sizeof(config->buffer_interval));
	config->buffer_interval = ntoh16(config->buffer_interval);

	if (request->body.len != request->body.pos) {
		debug("body len %u does not match body pos %u\n", request->body.len, request->body.pos);
		return -1;
	}

	return 0;
}

int config_validate(config_t *config) {
	if (config->reading_interval < 8 || config->reading_interval > 4096) {
		debug("invalid reading interval %hu on config\n", config->reading_interval);
		return -1;
	}

	if (config->metric_interval < 8 || config->metric_interval > 4096) {
		debug("invalid metric interval %hu on config\n", config->metric_interval);
		return -1;
	}

	if (config->buffer_interval < 8 || config->buffer_interval > 4096) {
		debug("invalid buffer interval %hu on config\n", config->buffer_interval);
		return -1;
	}

	return 0;
}

uint16_t config_insert(octet_t *db, config_t *config) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*config->id); index++) {
		(*config->id)[index] = (uint8_t)(rand() & 0xff);
	}

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), config->device_id, sizeof(*config->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, config_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert config for device %02x%02x captured at %lu\n", (*config->device_id)[0], (*config->device_id)[1],
				config->captured_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - config_row.size, db->row, config_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t captured_at = (time_t)octet_uint64_read(db->row, config_row.captured_at);
		if (captured_at <= config->captured_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, config_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= config_row.size;
	}

	octet_blob_write(db->row, config_row.id, (uint8_t *)config->id, sizeof(*config->id));
	octet_uint8_write(db->row, config_row.led_debug, config->led_debug);
	octet_uint8_write(db->row, config_row.reading_enable, config->reading_enable);
	octet_uint8_write(db->row, config_row.metric_enable, config->metric_enable);
	octet_uint8_write(db->row, config_row.buffer_enable, config->buffer_enable);
	octet_uint16_write(db->row, config_row.reading_interval, config->reading_interval);
	octet_uint16_write(db->row, config_row.metric_interval, config->metric_interval);
	octet_uint16_write(db->row, config_row.buffer_interval, config->buffer_interval);
	octet_uint64_write(db->row, config_row.captured_at, (uint64_t)config->captured_at);
	octet_blob_write(db->row, config_row.uplink_id, (uint8_t *)config->uplink_id, sizeof(*config->uplink_id));

	if (octet_row_write(&stmt, file, offset, db->row, config_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void config_find_one_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	uint8_t id[16];
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

	status = config_select_one_by_device(db, &device, response);
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
	info("found config for device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}

void config_modify(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	uint8_t id[16];
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

	config_t config;
	if (request->body.len == 0 || config_parse(&config, request) == -1 || config_validate(&config) == -1) {
		response->status = 400;
		return;
	}

	info("updated config for device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 202;
}
