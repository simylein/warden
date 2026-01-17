#include "config.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "database.h"
#include "device.h"
#include "user-device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *config_table = "config";
const char *config_schema = "create table config ("
														"id blob primary key, "
														"led_debug boolean not null, "
														"reading_enable boolean not null, "
														"metric_enable boolean not null, "
														"buffer_enable boolean not null, "
														"reading_interval integer not null, "
														"metric_interval integer not null, "
														"buffer_interval integer not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"device_id blob not null, "
														"foreign key (uplink_id) references uplink(id) on delete cascade, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";

uint16_t config_select_one_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"config.led_debug, config.reading_enable, config.metric_enable, config.buffer_enable, "
										"config.reading_interval, config.metric_interval, config.buffer_interval, config.captured_at "
										"from config "
										"join user_device on user_device.device_id = config.device_id and user_device.user_id = ? "
										"where config.device_id = ? "
										"order by captured_at desc "
										"limit 1";
	debug("select config for device %02x%02x for user %02x%02x\n", (*device->id)[0], (*device->id)[1], bwt->id[0], bwt->id[1]);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, device->id, sizeof(*device->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const bool led_debug = (bool)sqlite3_column_int(stmt, 0);
		const bool reading_enable = (bool)sqlite3_column_int(stmt, 1);
		const bool metric_enable = (bool)sqlite3_column_int(stmt, 2);
		const bool buffer_enable = (bool)sqlite3_column_int(stmt, 3);
		const uint16_t reading_interval = (uint16_t)sqlite3_column_int(stmt, 4);
		const uint16_t metric_interval = (uint16_t)sqlite3_column_int(stmt, 5);
		const uint16_t buffer_interval = (uint16_t)sqlite3_column_int(stmt, 6);
		const time_t captured_at = (time_t)sqlite3_column_int64(stmt, 7);
		body_write(response, &led_debug, sizeof(led_debug));
		body_write(response, &reading_enable, sizeof(reading_enable));
		body_write(response, &metric_enable, sizeof(metric_enable));
		body_write(response, &buffer_enable, sizeof(buffer_enable));
		body_write(response, (uint16_t[]){hton16((uint16_t)reading_interval)}, sizeof(reading_interval));
		body_write(response, (uint16_t[]){hton16((uint16_t)metric_interval)}, sizeof(metric_interval));
		body_write(response, (uint16_t[]){hton16((uint16_t)buffer_interval)}, sizeof(buffer_interval));
		body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
		status = 0;
	} else if (result == SQLITE_DONE) {
		debug("no config for device %02x%02x found\n", (*device->id)[0], (*device->id)[1]);
		status = 204;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t config_insert(sqlite3 *database, config_t *config) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into config (id, led_debug, reading_enable, metric_enable, buffer_enable, "
										"reading_interval, metric_interval, buffer_interval, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("insert config for device %02x%02x captured at %lu\n", (*config->device_id)[0], (*config->device_id)[1],
				config->captured_at);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, config->led_debug);
	sqlite3_bind_int(stmt, 2, config->reading_enable);
	sqlite3_bind_int(stmt, 3, config->metric_enable);
	sqlite3_bind_int(stmt, 4, config->buffer_enable);
	sqlite3_bind_int(stmt, 5, config->reading_interval);
	sqlite3_bind_int(stmt, 6, config->metric_interval);
	sqlite3_bind_int(stmt, 7, config->buffer_interval);
	sqlite3_bind_int64(stmt, 8, config->captured_at);
	sqlite3_bind_blob(stmt, 9, config->uplink_id, sizeof(*config->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 10, config->device_id, sizeof(*config->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*config->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*config->id));
			status = 500;
			goto cleanup;
		}
		memcpy(config->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("config is conflicting because %s\n", sqlite3_errmsg(database));
		status = 409;
		goto cleanup;
	} else {
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void config_find_one_by_device(octet_t *db, sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	status = config_select_one_by_device(database, bwt, &device, response);
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
