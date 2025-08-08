#include "device.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "metric.h"
#include "reading.h"
#include "uplink.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *device_table = "device";
const char *device_schema = "create table device ("
														"id blob primary key, "
														"name text not null unique, "
														"type text not null, "
														"created_at datetime not null, "
														"updated_at datetime"
														")";

uint16_t device_existing(sqlite3 *database, bwt_t *bwt, device_t *device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"device.id, "
										"user_device.device_id "
										"from device "
										"left join user_device on user_device.device_id = device.id and user_device.user_id = ? "
										"where device.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, device->id, sizeof(*device->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*device->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*device->id));
			status = 500;
			goto cleanup;
		}
		const int user_device_device_id_type = sqlite3_column_type(stmt, 1);
		if (user_device_device_id_type == SQLITE_NULL) {
			status = 403;
			goto cleanup;
		}
		memcpy(device->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
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

uint16_t device_select(sqlite3 *database, bwt_t *bwt, response_t *response, uint8_t *devices_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"device.id, device.name, device.type, device.created_at, device.updated_at, "
			"uplink.id, uplink.received_at, "
			"reading.id, reading.temperature, reading.humidity, reading.captured_at, "
			"metric.id, metric.photovoltaic, metric.battery, metric.captured_at "
			"from device "
			"join user_device on user_device.device_id = device.id and user_device.user_id = ? "
			"left join uplink on uplink.id = "
			"(select id from uplink where device_id = device.id order by uplink.received_at desc limit 1) "
			"left join reading on reading.id = "
			"(select reading.id from reading where reading.device_id = device.id order by reading.captured_at desc limit 1) "
			"left join metric on metric.id = "
			"(select metric.id from metric where metric.device_id = device.id order by metric.captured_at desc limit 1) "
			"order by device.name asc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const uint8_t *id = sqlite3_column_blob(stmt, 0);
			const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
			if (id_len != sizeof(*((device_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((device_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *name = sqlite3_column_text(stmt, 1);
			const size_t name_len = (size_t)sqlite3_column_bytes(stmt, 1);
			const uint8_t *type = sqlite3_column_text(stmt, 2);
			const size_t type_len = (size_t)sqlite3_column_bytes(stmt, 2);
			const time_t created_at = (time_t)sqlite3_column_int64(stmt, 3);
			const time_t updated_at = (time_t)sqlite3_column_int64(stmt, 4);
			const int updated_at_type = sqlite3_column_type(stmt, 4);
			const uint8_t *uplink_id = sqlite3_column_blob(stmt, 5);
			const size_t uplink_id_len = (size_t)sqlite3_column_bytes(stmt, 5);
			const int uplink_id_type = sqlite3_column_type(stmt, 5);
			if (uplink_id_type != SQLITE_NULL && uplink_id_len != sizeof(*((uplink_t *)0)->id)) {
				error("uplink id length %zu does not match buffer length %zu\n", uplink_id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const time_t uplink_received_at = (time_t)sqlite3_column_int64(stmt, 6);
			const int uplink_received_at_type = sqlite3_column_type(stmt, 6);
			const uint8_t *reading_id = sqlite3_column_blob(stmt, 7);
			const size_t reading_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
			const int reading_id_type = sqlite3_column_type(stmt, 7);
			if (reading_id_type != SQLITE_NULL && reading_id_len != sizeof(*((reading_t *)0)->id)) {
				error("reading id length %zu does not match buffer length %zu\n", reading_id_len, sizeof(*((reading_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const double reading_temperature = sqlite3_column_double(stmt, 8);
			const int reading_temperature_type = sqlite3_column_type(stmt, 8);
			const double reading_humidity = sqlite3_column_double(stmt, 9);
			const int reading_humidity_type = sqlite3_column_type(stmt, 9);
			const time_t reading_captured_at = (time_t)sqlite3_column_int64(stmt, 10);
			const int reading_captured_at_type = sqlite3_column_type(stmt, 10);
			const uint8_t *metric_id = sqlite3_column_blob(stmt, 11);
			const size_t metric_id_len = (size_t)sqlite3_column_bytes(stmt, 11);
			const int metric_id_type = sqlite3_column_type(stmt, 11);
			if (metric_id_type != SQLITE_NULL && metric_id_len != sizeof(*((metric_t *)0)->id)) {
				error("metric id length %zu does not match buffer length %zu\n", metric_id_len, sizeof(*((metric_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const double metric_photovoltaic = sqlite3_column_double(stmt, 12);
			const int metric_photovoltaic_type = sqlite3_column_type(stmt, 12);
			const double metric_battery = sqlite3_column_double(stmt, 13);
			const int metric_battery_type = sqlite3_column_type(stmt, 13);
			const time_t metric_captured_at = (time_t)sqlite3_column_int64(stmt, 14);
			const int metric_captured_at_type = sqlite3_column_type(stmt, 14);
			append_body(response, id, id_len);
			append_body(response, name, name_len);
			append_body(response, (char[]){0x00}, sizeof(char));
			append_body(response, type, type_len);
			append_body(response, (char[]){0x00}, sizeof(char));
			append_body(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
			append_body(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
			if (updated_at_type != SQLITE_NULL) {
				append_body(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
			}
			append_body(response, (char[]){uplink_id_type != SQLITE_NULL}, sizeof(char));
			if (uplink_id_type != SQLITE_NULL) {
				append_body(response, uplink_id, uplink_id_len);
			}
			append_body(response, (char[]){uplink_received_at_type != SQLITE_NULL}, sizeof(char));
			if (uplink_received_at_type != SQLITE_NULL) {
				append_body(response, (uint64_t[]){hton64((uint64_t)uplink_received_at)}, sizeof(uplink_received_at));
			}
			append_body(response, (char[]){reading_id_type != SQLITE_NULL}, sizeof(char));
			if (reading_id_type != SQLITE_NULL) {
				append_body(response, reading_id, reading_id_len);
			}
			append_body(response, (char[]){reading_temperature_type != SQLITE_NULL}, sizeof(char));
			if (reading_temperature_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(int16_t)(reading_temperature * 100))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){reading_humidity_type != SQLITE_NULL}, sizeof(char));
			if (reading_humidity_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(reading_humidity * 100))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){reading_captured_at_type != SQLITE_NULL}, sizeof(char));
			if (reading_captured_at_type != SQLITE_NULL) {
				append_body(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
			}
			append_body(response, (char[]){metric_id_type != SQLITE_NULL}, sizeof(char));
			if (metric_id_type != SQLITE_NULL) {
				append_body(response, metric_id, metric_id_len);
			}
			append_body(response, (char[]){metric_photovoltaic_type != SQLITE_NULL}, sizeof(char));
			if (metric_photovoltaic_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(metric_photovoltaic * 1000))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){metric_battery_type != SQLITE_NULL}, sizeof(char));
			if (metric_battery_type != SQLITE_NULL) {
				append_body(response, (uint16_t[]){hton16((uint16_t)(metric_battery * 1000))}, sizeof(uint16_t));
			}
			append_body(response, (char[]){metric_captured_at_type != SQLITE_NULL}, sizeof(char));
			if (metric_captured_at_type != SQLITE_NULL) {
				append_body(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
			}
			*devices_len += 1;
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

uint16_t device_select_one(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"device.id, device.name, device.type, device.created_at, device.updated_at, "
			"uplink.id, uplink.received_at, "
			"reading.id, reading.temperature, reading.humidity, reading.captured_at, "
			"metric.id, metric.photovoltaic, metric.battery, metric.captured_at "
			"from device "
			"join user_device on user_device.device_id = device.id and user_device.user_id = ? "
			"left join uplink on uplink.id = "
			"(select id from uplink where device_id = device.id order by uplink.received_at desc limit 1) "
			"left join reading on reading.id = "
			"(select reading.id from reading where reading.device_id = device.id order by reading.captured_at desc limit 1) "
			"left join metric on metric.id = "
			"(select metric.id from metric where metric.device_id = device.id order by metric.captured_at desc limit 1) "
			"where device.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, device->id, sizeof(*device->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*device->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*device->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t *name = sqlite3_column_text(stmt, 1);
		const size_t name_len = (size_t)sqlite3_column_bytes(stmt, 1);
		const uint8_t *type = sqlite3_column_text(stmt, 2);
		const size_t type_len = (size_t)sqlite3_column_bytes(stmt, 2);
		const uint64_t created_at = (uint64_t)sqlite3_column_int64(stmt, 3);
		const uint64_t updated_at = (uint64_t)sqlite3_column_int64(stmt, 4);
		const int updated_at_type = sqlite3_column_type(stmt, 4);
		const uint8_t *uplink_id = sqlite3_column_blob(stmt, 5);
		const size_t uplink_id_len = (size_t)sqlite3_column_bytes(stmt, 5);
		const int uplink_id_type = sqlite3_column_type(stmt, 5);
		if (uplink_id_type != SQLITE_NULL && uplink_id_len != sizeof(*((uplink_t *)0)->id)) {
			error("uplink id length %zu does not match buffer length %zu\n", uplink_id_len, sizeof(*((uplink_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const uint64_t uplink_received_at = (uint64_t)sqlite3_column_int64(stmt, 6);
		const int uplink_received_at_type = sqlite3_column_type(stmt, 6);
		const uint8_t *reading_id = sqlite3_column_blob(stmt, 7);
		const size_t reading_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
		const int reading_id_type = sqlite3_column_type(stmt, 7);
		if (reading_id_type != SQLITE_NULL && reading_id_len != sizeof(*((reading_t *)0)->id)) {
			error("reading id length %zu does not match buffer length %zu\n", reading_id_len, sizeof(*((reading_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const double reading_temperature = sqlite3_column_double(stmt, 8);
		const int reading_temperature_type = sqlite3_column_type(stmt, 8);
		const double reading_humidity = sqlite3_column_double(stmt, 9);
		const int reading_humidity_type = sqlite3_column_type(stmt, 9);
		const uint64_t reading_captured_at = (uint64_t)sqlite3_column_int64(stmt, 10);
		const int reading_captured_at_type = sqlite3_column_type(stmt, 10);
		const uint8_t *metric_id = sqlite3_column_blob(stmt, 11);
		const size_t metric_id_len = (size_t)sqlite3_column_bytes(stmt, 11);
		const int metric_id_type = sqlite3_column_type(stmt, 11);
		if (metric_id_type != SQLITE_NULL && metric_id_len != sizeof(*((metric_t *)0)->id)) {
			error("metric id length %zu does not match buffer length %zu\n", metric_id_len, sizeof(*((metric_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const double metric_photovoltaic = sqlite3_column_double(stmt, 12);
		const int metric_photovoltaic_type = sqlite3_column_type(stmt, 12);
		const double metric_battery = sqlite3_column_double(stmt, 13);
		const int metric_battery_type = sqlite3_column_type(stmt, 13);
		const uint64_t metric_captured_at = (uint64_t)sqlite3_column_int64(stmt, 14);
		const int metric_captured_at_type = sqlite3_column_type(stmt, 14);
		append_body(response, id, id_len);
		append_body(response, name, name_len);
		append_body(response, (char[]){0x00}, sizeof(char));
		append_body(response, type, type_len);
		append_body(response, (char[]){0x00}, sizeof(char));
		append_body(response, (uint64_t[]){hton64(created_at)}, sizeof(created_at));
		append_body(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
		if (updated_at_type != SQLITE_NULL) {
			append_body(response, (uint64_t[]){hton64(updated_at)}, sizeof(updated_at));
		}
		append_body(response, (char[]){uplink_id_type != SQLITE_NULL}, sizeof(char));
		if (uplink_id_type != SQLITE_NULL) {
			append_body(response, uplink_id, uplink_id_len);
		}
		append_body(response, (char[]){uplink_received_at_type != SQLITE_NULL}, sizeof(char));
		if (uplink_received_at_type != SQLITE_NULL) {
			append_body(response, (uint64_t[]){hton64(uplink_received_at)}, sizeof(uplink_received_at));
		}
		append_body(response, (char[]){reading_id_type != SQLITE_NULL}, sizeof(char));
		if (reading_id_type != SQLITE_NULL) {
			append_body(response, reading_id, reading_id_len);
		}
		append_body(response, (char[]){reading_temperature_type != SQLITE_NULL}, sizeof(char));
		if (reading_temperature_type != SQLITE_NULL) {
			append_body(response, (uint16_t[]){hton16((uint16_t)(int16_t)(reading_temperature * 100))}, sizeof(uint16_t));
		}
		append_body(response, (char[]){reading_humidity_type != SQLITE_NULL}, sizeof(char));
		if (reading_humidity_type != SQLITE_NULL) {
			append_body(response, (uint16_t[]){hton16((uint16_t)(reading_humidity * 100))}, sizeof(uint16_t));
		}
		append_body(response, (char[]){reading_captured_at_type != SQLITE_NULL}, sizeof(char));
		if (reading_captured_at_type != SQLITE_NULL) {
			append_body(response, (uint64_t[]){hton64(reading_captured_at)}, sizeof(reading_captured_at));
		}
		append_body(response, (char[]){metric_id_type != SQLITE_NULL}, sizeof(char));
		if (metric_id_type != SQLITE_NULL) {
			append_body(response, metric_id, metric_id_len);
		}
		append_body(response, (char[]){metric_photovoltaic_type != SQLITE_NULL}, sizeof(char));
		if (metric_photovoltaic_type != SQLITE_NULL) {
			append_body(response, (uint16_t[]){hton16((uint16_t)(metric_photovoltaic * 1000))}, sizeof(uint16_t));
		}
		append_body(response, (char[]){metric_battery_type != SQLITE_NULL}, sizeof(char));
		if (metric_battery_type != SQLITE_NULL) {
			append_body(response, (uint16_t[]){hton16((uint16_t)(metric_battery * 1000))}, sizeof(uint16_t));
		}
		append_body(response, (char[]){metric_captured_at_type != SQLITE_NULL}, sizeof(char));
		if (metric_captured_at_type != SQLITE_NULL) {
			append_body(response, (uint64_t[]){hton64(metric_captured_at)}, sizeof(metric_captured_at));
		}
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
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

uint16_t device_insert(sqlite3 *database, device_t *device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into device (id, name, type, created_at) "
										"values (randomblob(16), ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, device->name, device->name_len, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, device->type, device->type_len, SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, time(NULL));

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*device->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*device->id));
			status = 500;
			goto cleanup;
		}
		memcpy(device->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("device name %.*s already taken\n", (int)device->name_len, device->name);
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

void device_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t devices_len = 0;
	uint16_t status = device_select(database, bwt, response, &devices_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found %hhu devices\n", devices_len);
	response->status = 200;
}

void device_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 12, &uuid_len);
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
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = device_select_one(database, bwt, &device, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}
