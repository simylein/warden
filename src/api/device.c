#include "device.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "buffer.h"
#include "database.h"
#include "metric.h"
#include "reading.h"
#include "uplink.h"
#include "user.h"
#include "zone.h"
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
														"zone_id blob, "
														"firmware text, "
														"hardware text, "
														"created_at datetime not null, "
														"updated_at datetime, "
														"foreign key (zone_id) references zone(id) on delete set null"
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
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t device_select(sqlite3 *database, bwt_t *bwt, device_query_t *query, response_t *response, uint8_t *devices_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"device.id, device.name, device.created_at, device.updated_at, "
			"zone.id, zone.name, zone.color, "
			"reading.id, reading.temperature, reading.humidity, reading.captured_at, "
			"metric.id, metric.photovoltaic, metric.battery, metric.captured_at, "
			"buffer.id, buffer.delay, buffer.level, buffer.captured_at, "
			"uplink.id, uplink.received_at "
			"from device "
			"join user_device on user_device.device_id = device.id and user_device.user_id = ?1 "
			"left join zone on zone.id = device.zone_id "
			"left join reading on reading.id = "
			"(select reading.id from reading where reading.device_id = device.id order by reading.captured_at desc limit 1) "
			"left join metric on metric.id = "
			"(select metric.id from metric where metric.device_id = device.id order by metric.captured_at desc limit 1) "
			"left join buffer on buffer.id = "
			"(select buffer.id from buffer where buffer.device_id = device.id order by buffer.captured_at desc limit 1) "
			"left join uplink on uplink.id = "
			"(select id from uplink where device_id = device.id order by uplink.received_at desc limit 1) "
			"order by "
			"case when ?2 = 'id' and ?3 = 'asc' then device.id end asc, "
			"case when ?2 = 'id' and ?3 = 'desc' then device.id end desc, "
			"case when ?2 = 'name' and ?3 = 'asc' then device.name end asc, "
			"case when ?2 = 'name' and ?3 = 'desc' then device.name end desc, "
			"case when ?2 = 'zone' and ?3 = 'asc' then zone.name end asc, "
			"case when ?2 = 'zone' and ?3 = 'desc' then zone.name end desc, "
			"case when ?2 = 'temperature' and ?3 = 'asc' then reading.temperature end asc, "
			"case when ?2 = 'temperature' and ?3 = 'desc' then reading.temperature end desc, "
			"case when ?2 = 'humidity' and ?3 = 'asc' then reading.humidity end asc, "
			"case when ?2 = 'humidity' and ?3 = 'desc' then reading.humidity end desc, "
			"case when ?2 = 'photovoltaic' and ?3 = 'asc' then metric.photovoltaic end asc, "
			"case when ?2 = 'photovoltaic' and ?3 = 'desc' then metric.photovoltaic end desc, "
			"case when ?2 = 'battery' and ?3 = 'asc' then metric.battery end asc, "
			"case when ?2 = 'battery' and ?3 = 'desc' then metric.battery end desc, "
			"case when ?2 = 'delay' and ?3 = 'asc' then buffer.delay end asc, "
			"case when ?2 = 'delay' and ?3 = 'desc' then buffer.delay end desc, "
			"case when ?2 = 'level' and ?3 = 'asc' then buffer.level end asc, "
			"case when ?2 = 'level' and ?3 = 'desc' then buffer.level end desc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, query->order, query->order_len, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, query->sort, query->sort_len, SQLITE_STATIC);

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
			const time_t created_at = (time_t)sqlite3_column_int64(stmt, 2);
			const time_t updated_at = (time_t)sqlite3_column_int64(stmt, 3);
			const int updated_at_type = sqlite3_column_type(stmt, 3);
			const uint8_t *zone_id = sqlite3_column_blob(stmt, 4);
			const size_t zone_id_len = (size_t)sqlite3_column_bytes(stmt, 4);
			const int zone_id_type = sqlite3_column_type(stmt, 4);
			if (zone_id_type != SQLITE_NULL && zone_id_len != sizeof(*((zone_t *)0)->id)) {
				error("zone id length %zu does not match buffer length %zu\n", zone_id_len, sizeof(*((zone_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *zone_name = sqlite3_column_text(stmt, 5);
			const size_t zone_name_len = (size_t)sqlite3_column_bytes(stmt, 5);
			const int zone_name_type = sqlite3_column_type(stmt, 5);
			const uint8_t *zone_color = sqlite3_column_blob(stmt, 6);
			const size_t zone_color_len = (size_t)sqlite3_column_bytes(stmt, 6);
			const int zone_color_type = sqlite3_column_type(stmt, 6);
			if (zone_color_type != SQLITE_NULL && zone_color_len != sizeof(*((zone_t *)0)->color)) {
				error("zone color length %zu does not match buffer length %zu\n", zone_color_len, sizeof(*((zone_t *)0)->color));
				status = 500;
				goto cleanup;
			}
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
			const uint8_t *buffer_id = sqlite3_column_blob(stmt, 15);
			const size_t buffer_id_len = (size_t)sqlite3_column_bytes(stmt, 15);
			const int buffer_id_type = sqlite3_column_type(stmt, 15);
			if (buffer_id_type != SQLITE_NULL && buffer_id_len != sizeof(*((buffer_t *)0)->id)) {
				error("buffer id length %zu does not match buffer length %zu\n", buffer_id_len, sizeof(*((buffer_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint32_t buffer_delay = (uint32_t)sqlite3_column_int(stmt, 16);
			const int buffer_delay_type = sqlite3_column_type(stmt, 16);
			const uint16_t buffer_level = (uint16_t)sqlite3_column_int(stmt, 17);
			const int buffer_level_type = sqlite3_column_type(stmt, 17);
			const time_t buffer_captured_at = (time_t)sqlite3_column_int64(stmt, 18);
			const int buffer_captured_at_type = sqlite3_column_type(stmt, 18);
			const uint8_t *uplink_id = sqlite3_column_blob(stmt, 19);
			const size_t uplink_id_len = (size_t)sqlite3_column_bytes(stmt, 19);
			const int uplink_id_type = sqlite3_column_type(stmt, 19);
			if (uplink_id_type != SQLITE_NULL && uplink_id_len != sizeof(*((uplink_t *)0)->id)) {
				error("uplink id length %zu does not match buffer length %zu\n", uplink_id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const time_t uplink_received_at = (time_t)sqlite3_column_int64(stmt, 20);
			const int uplink_received_at_type = sqlite3_column_type(stmt, 20);
			body_write(response, id, id_len);
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
			body_write(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
			if (updated_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
			}
			body_write(response, (char[]){zone_id_type != SQLITE_NULL}, sizeof(char));
			if (zone_id_type != SQLITE_NULL) {
				body_write(response, zone_id, zone_id_len);
			}
			body_write(response, (char[]){zone_name_type != SQLITE_NULL}, sizeof(char));
			if (zone_name_type != SQLITE_NULL) {
				body_write(response, zone_name, zone_name_len);
				body_write(response, (char[]){0x00}, sizeof(char));
			}
			body_write(response, (char[]){zone_color_type != SQLITE_NULL}, sizeof(char));
			if (zone_color_type != SQLITE_NULL) {
				body_write(response, zone_color, zone_color_len);
			}
			body_write(response, (char[]){reading_id_type != SQLITE_NULL}, sizeof(char));
			if (reading_id_type != SQLITE_NULL) {
				body_write(response, reading_id, reading_id_len);
			}
			body_write(response, (char[]){reading_temperature_type != SQLITE_NULL}, sizeof(char));
			if (reading_temperature_type != SQLITE_NULL) {
				body_write(response, (uint16_t[]){hton16((uint16_t)(int16_t)(reading_temperature * 100))}, sizeof(uint16_t));
			}
			body_write(response, (char[]){reading_humidity_type != SQLITE_NULL}, sizeof(char));
			if (reading_humidity_type != SQLITE_NULL) {
				body_write(response, (uint16_t[]){hton16((uint16_t)(reading_humidity * 100))}, sizeof(uint16_t));
			}
			body_write(response, (char[]){reading_captured_at_type != SQLITE_NULL}, sizeof(char));
			if (reading_captured_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
			}
			body_write(response, (char[]){metric_id_type != SQLITE_NULL}, sizeof(char));
			if (metric_id_type != SQLITE_NULL) {
				body_write(response, metric_id, metric_id_len);
			}
			body_write(response, (char[]){metric_photovoltaic_type != SQLITE_NULL}, sizeof(char));
			if (metric_photovoltaic_type != SQLITE_NULL) {
				body_write(response, (uint16_t[]){hton16((uint16_t)(metric_photovoltaic * 1000))}, sizeof(uint16_t));
			}
			body_write(response, (char[]){metric_battery_type != SQLITE_NULL}, sizeof(char));
			if (metric_battery_type != SQLITE_NULL) {
				body_write(response, (uint16_t[]){hton16((uint16_t)(metric_battery * 1000))}, sizeof(uint16_t));
			}
			body_write(response, (char[]){metric_captured_at_type != SQLITE_NULL}, sizeof(char));
			if (metric_captured_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
			}
			body_write(response, (char[]){buffer_id_type != SQLITE_NULL}, sizeof(char));
			if (buffer_id_type != SQLITE_NULL) {
				body_write(response, buffer_id, buffer_id_len);
			}
			body_write(response, (char[]){buffer_delay_type != SQLITE_NULL}, sizeof(char));
			if (buffer_delay_type != SQLITE_NULL) {
				body_write(response, (uint32_t[]){hton32(buffer_delay)}, sizeof(buffer_delay));
			}
			body_write(response, (char[]){buffer_level_type != SQLITE_NULL}, sizeof(char));
			if (buffer_level_type != SQLITE_NULL) {
				body_write(response, (uint16_t[]){hton16(buffer_level)}, sizeof(buffer_level));
			}
			body_write(response, (char[]){buffer_captured_at_type != SQLITE_NULL}, sizeof(char));
			if (buffer_captured_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)buffer_captured_at)}, sizeof(buffer_captured_at));
			}
			body_write(response, (char[]){uplink_id_type != SQLITE_NULL}, sizeof(char));
			if (uplink_id_type != SQLITE_NULL) {
				body_write(response, uplink_id, uplink_id_len);
			}
			body_write(response, (char[]){uplink_received_at_type != SQLITE_NULL}, sizeof(char));
			if (uplink_received_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)uplink_received_at)}, sizeof(uplink_received_at));
			}
			*devices_len += 1;
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

uint16_t device_select_one(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"device.id, device.name, device.firmware, device.hardware, device.created_at, device.updated_at, "
			"zone.id, zone.name, zone.color, "
			"reading.id, reading.temperature, reading.humidity, reading.captured_at, "
			"metric.id, metric.photovoltaic, metric.battery, metric.captured_at, "
			"buffer.id, buffer.delay, buffer.level, buffer.captured_at, "
			"uplink.id, uplink.received_at "
			"from device "
			"join user_device on user_device.device_id = device.id and user_device.user_id = ? "
			"left join zone on zone.id = device.zone_id "
			"left join reading on reading.id = "
			"(select reading.id from reading where reading.device_id = device.id order by reading.captured_at desc limit 1) "
			"left join metric on metric.id = "
			"(select metric.id from metric where metric.device_id = device.id order by metric.captured_at desc limit 1) "
			"left join buffer on buffer.id = "
			"(select buffer.id from buffer where buffer.device_id = device.id order by buffer.captured_at desc limit 1) "
			"left join uplink on uplink.id = "
			"(select id from uplink where device_id = device.id order by uplink.received_at desc limit 1) "
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
		if (id_len != sizeof(*((device_t *)0)->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((device_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t *name = sqlite3_column_text(stmt, 1);
		const size_t name_len = (size_t)sqlite3_column_bytes(stmt, 1);
		const uint8_t *firmware = sqlite3_column_text(stmt, 2);
		const size_t firmware_len = (size_t)sqlite3_column_bytes(stmt, 2);
		const int firmware_type = sqlite3_column_type(stmt, 2);
		const uint8_t *hardware = sqlite3_column_text(stmt, 3);
		const size_t hardware_len = (size_t)sqlite3_column_bytes(stmt, 3);
		const int hardware_type = sqlite3_column_type(stmt, 3);
		const time_t created_at = (time_t)sqlite3_column_int64(stmt, 4);
		const time_t updated_at = (time_t)sqlite3_column_int64(stmt, 5);
		const int updated_at_type = sqlite3_column_type(stmt, 5);
		const uint8_t *zone_id = sqlite3_column_blob(stmt, 6);
		const size_t zone_id_len = (size_t)sqlite3_column_bytes(stmt, 6);
		const int zone_id_type = sqlite3_column_type(stmt, 6);
		if (zone_id_type != SQLITE_NULL && zone_id_len != sizeof(*((zone_t *)0)->id)) {
			error("zone id length %zu does not match buffer length %zu\n", zone_id_len, sizeof(*((zone_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t *zone_name = sqlite3_column_text(stmt, 7);
		const size_t zone_name_len = (size_t)sqlite3_column_bytes(stmt, 7);
		const int zone_name_type = sqlite3_column_type(stmt, 7);
		const uint8_t *zone_color = sqlite3_column_blob(stmt, 8);
		const size_t zone_color_len = (size_t)sqlite3_column_bytes(stmt, 8);
		const int zone_color_type = sqlite3_column_type(stmt, 8);
		const uint8_t *reading_id = sqlite3_column_blob(stmt, 9);
		const size_t reading_id_len = (size_t)sqlite3_column_bytes(stmt, 9);
		const int reading_id_type = sqlite3_column_type(stmt, 9);
		if (reading_id_type != SQLITE_NULL && reading_id_len != sizeof(*((reading_t *)0)->id)) {
			error("reading id length %zu does not match buffer length %zu\n", reading_id_len, sizeof(*((reading_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const double reading_temperature = sqlite3_column_double(stmt, 10);
		const int reading_temperature_type = sqlite3_column_type(stmt, 10);
		const double reading_humidity = sqlite3_column_double(stmt, 11);
		const int reading_humidity_type = sqlite3_column_type(stmt, 11);
		const time_t reading_captured_at = (time_t)sqlite3_column_int64(stmt, 12);
		const int reading_captured_at_type = sqlite3_column_type(stmt, 12);
		const uint8_t *metric_id = sqlite3_column_blob(stmt, 13);
		const size_t metric_id_len = (size_t)sqlite3_column_bytes(stmt, 13);
		const int metric_id_type = sqlite3_column_type(stmt, 13);
		if (metric_id_type != SQLITE_NULL && metric_id_len != sizeof(*((metric_t *)0)->id)) {
			error("metric id length %zu does not match buffer length %zu\n", metric_id_len, sizeof(*((metric_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const double metric_photovoltaic = sqlite3_column_double(stmt, 14);
		const int metric_photovoltaic_type = sqlite3_column_type(stmt, 14);
		const double metric_battery = sqlite3_column_double(stmt, 15);
		const int metric_battery_type = sqlite3_column_type(stmt, 15);
		const time_t metric_captured_at = (time_t)sqlite3_column_int64(stmt, 16);
		const int metric_captured_at_type = sqlite3_column_type(stmt, 16);
		const uint8_t *buffer_id = sqlite3_column_blob(stmt, 17);
		const size_t buffer_id_len = (size_t)sqlite3_column_bytes(stmt, 17);
		const int buffer_id_type = sqlite3_column_type(stmt, 17);
		if (buffer_id_type != SQLITE_NULL && buffer_id_len != sizeof(*((buffer_t *)0)->id)) {
			error("buffer id length %zu does not match buffer length %zu\n", buffer_id_len, sizeof(*((buffer_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const uint32_t buffer_delay = (uint32_t)sqlite3_column_int(stmt, 18);
		const int buffer_delay_type = sqlite3_column_type(stmt, 18);
		const uint16_t buffer_level = (uint16_t)sqlite3_column_int(stmt, 19);
		const int buffer_level_type = sqlite3_column_type(stmt, 19);
		const time_t buffer_captured_at = (time_t)sqlite3_column_int64(stmt, 20);
		const int buffer_captured_at_type = sqlite3_column_type(stmt, 20);
		const uint8_t *uplink_id = sqlite3_column_blob(stmt, 21);
		const size_t uplink_id_len = (size_t)sqlite3_column_bytes(stmt, 21);
		const int uplink_id_type = sqlite3_column_type(stmt, 21);
		if (uplink_id_type != SQLITE_NULL && uplink_id_len != sizeof(*((uplink_t *)0)->id)) {
			error("uplink id length %zu does not match buffer length %zu\n", uplink_id_len, sizeof(*((uplink_t *)0)->id));
			status = 500;
			goto cleanup;
		}
		const time_t uplink_received_at = (time_t)sqlite3_column_int64(stmt, 22);
		const int uplink_received_at_type = sqlite3_column_type(stmt, 22);
		body_write(response, id, id_len);
		body_write(response, name, name_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, (char[]){firmware_type != SQLITE_NULL}, sizeof(char));
		if (firmware_type != SQLITE_NULL) {
			body_write(response, firmware, firmware_len);
			body_write(response, (char[]){0x00}, sizeof(char));
		}
		body_write(response, (char[]){hardware_type != SQLITE_NULL}, sizeof(char));
		if (hardware_type != SQLITE_NULL) {
			body_write(response, hardware, hardware_len);
			body_write(response, (char[]){0x00}, sizeof(char));
		}
		body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
		body_write(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
		if (updated_at_type != SQLITE_NULL) {
			body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
		}
		body_write(response, (char[]){zone_id_type != SQLITE_NULL}, sizeof(char));
		if (zone_id_type != SQLITE_NULL) {
			body_write(response, zone_id, zone_id_len);
		}
		body_write(response, (char[]){zone_name_type != SQLITE_NULL}, sizeof(char));
		if (zone_name_type != SQLITE_NULL) {
			body_write(response, zone_name, zone_name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
		}
		body_write(response, (char[]){zone_color_type != SQLITE_NULL}, sizeof(char));
		if (zone_color_type != SQLITE_NULL) {
			body_write(response, zone_color, zone_color_len);
		}
		body_write(response, (char[]){reading_id_type != SQLITE_NULL}, sizeof(char));
		if (reading_id_type != SQLITE_NULL) {
			body_write(response, reading_id, reading_id_len);
		}
		body_write(response, (char[]){reading_temperature_type != SQLITE_NULL}, sizeof(char));
		if (reading_temperature_type != SQLITE_NULL) {
			body_write(response, (uint16_t[]){hton16((uint16_t)(int16_t)(reading_temperature * 100))}, sizeof(uint16_t));
		}
		body_write(response, (char[]){reading_humidity_type != SQLITE_NULL}, sizeof(char));
		if (reading_humidity_type != SQLITE_NULL) {
			body_write(response, (uint16_t[]){hton16((uint16_t)(reading_humidity * 100))}, sizeof(uint16_t));
		}
		body_write(response, (char[]){reading_captured_at_type != SQLITE_NULL}, sizeof(char));
		if (reading_captured_at_type != SQLITE_NULL) {
			body_write(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
		}
		body_write(response, (char[]){metric_id_type != SQLITE_NULL}, sizeof(char));
		if (metric_id_type != SQLITE_NULL) {
			body_write(response, metric_id, metric_id_len);
		}
		body_write(response, (char[]){metric_photovoltaic_type != SQLITE_NULL}, sizeof(char));
		if (metric_photovoltaic_type != SQLITE_NULL) {
			body_write(response, (uint16_t[]){hton16((uint16_t)(metric_photovoltaic * 1000))}, sizeof(uint16_t));
		}
		body_write(response, (char[]){metric_battery_type != SQLITE_NULL}, sizeof(char));
		if (metric_battery_type != SQLITE_NULL) {
			body_write(response, (uint16_t[]){hton16((uint16_t)(metric_battery * 1000))}, sizeof(uint16_t));
		}
		body_write(response, (char[]){metric_captured_at_type != SQLITE_NULL}, sizeof(char));
		if (metric_captured_at_type != SQLITE_NULL) {
			body_write(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
		}
		body_write(response, (char[]){buffer_id_type != SQLITE_NULL}, sizeof(char));
		if (buffer_id_type != SQLITE_NULL) {
			body_write(response, buffer_id, buffer_id_len);
		}
		body_write(response, (char[]){buffer_delay_type != SQLITE_NULL}, sizeof(char));
		if (buffer_delay_type != SQLITE_NULL) {
			body_write(response, (uint32_t[]){hton32(buffer_delay)}, sizeof(buffer_delay));
		}
		body_write(response, (char[]){buffer_level_type != SQLITE_NULL}, sizeof(char));
		if (buffer_level_type != SQLITE_NULL) {
			body_write(response, (uint16_t[]){hton16(buffer_level)}, sizeof(buffer_level));
		}
		body_write(response, (char[]){buffer_captured_at_type != SQLITE_NULL}, sizeof(char));
		if (buffer_captured_at_type != SQLITE_NULL) {
			body_write(response, (uint64_t[]){hton64((uint64_t)buffer_captured_at)}, sizeof(buffer_captured_at));
		}
		body_write(response, (char[]){uplink_id_type != SQLITE_NULL}, sizeof(char));
		if (uplink_id_type != SQLITE_NULL) {
			body_write(response, uplink_id, uplink_id_len);
		}
		body_write(response, (char[]){uplink_received_at_type != SQLITE_NULL}, sizeof(char));
		if (uplink_received_at_type != SQLITE_NULL) {
			body_write(response, (uint64_t[]){hton64((uint64_t)uplink_received_at)}, sizeof(uplink_received_at));
		}
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
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

uint16_t device_select_by_user(sqlite3 *database, user_t *user, response_t *response, uint8_t *devices_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"device.id, device.name, device.created_at, device.updated_at, "
										"zone.id, zone.name, zone.color, "
										"uplink.id, uplink.received_at "
										"from device "
										"join user_device on user_device.device_id = device.id and user_device.user_id = ? "
										"left join zone on zone.id = device.zone_id "
										"left join uplink on uplink.id = "
										"(select id from uplink where device_id = device.id order by uplink.received_at desc limit 1) "
										"order by device.name asc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user->id, sizeof(*user->id), SQLITE_STATIC);

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
			const time_t created_at = (time_t)sqlite3_column_int64(stmt, 2);
			const time_t updated_at = (time_t)sqlite3_column_int64(stmt, 3);
			const int updated_at_type = sqlite3_column_type(stmt, 3);
			const uint8_t *zone_id = sqlite3_column_blob(stmt, 4);
			const size_t zone_id_len = (size_t)sqlite3_column_bytes(stmt, 4);
			const int zone_id_type = sqlite3_column_type(stmt, 4);
			if (zone_id_type != SQLITE_NULL && zone_id_len != sizeof(*((zone_t *)0)->id)) {
				error("zone id length %zu does not match buffer length %zu\n", zone_id_len, sizeof(*((zone_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *zone_name = sqlite3_column_text(stmt, 5);
			const size_t zone_name_len = (size_t)sqlite3_column_bytes(stmt, 5);
			const int zone_name_type = sqlite3_column_type(stmt, 5);
			const uint8_t *zone_color = sqlite3_column_blob(stmt, 6);
			const size_t zone_color_len = (size_t)sqlite3_column_bytes(stmt, 6);
			const int zone_color_type = sqlite3_column_type(stmt, 6);
			const uint8_t *uplink_id = sqlite3_column_blob(stmt, 7);
			const size_t uplink_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
			const int uplink_id_type = sqlite3_column_type(stmt, 7);
			if (uplink_id_type != SQLITE_NULL && uplink_id_len != sizeof(*((uplink_t *)0)->id)) {
				error("uplink id length %zu does not match buffer length %zu\n", uplink_id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const time_t uplink_received_at = (time_t)sqlite3_column_int64(stmt, 8);
			const int uplink_received_at_type = sqlite3_column_type(stmt, 8);
			body_write(response, id, id_len);
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
			body_write(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
			if (updated_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
			}
			body_write(response, (char[]){zone_id_type != SQLITE_NULL}, sizeof(char));
			if (zone_id_type != SQLITE_NULL) {
				body_write(response, zone_id, zone_id_len);
			}
			body_write(response, (char[]){zone_name_type != SQLITE_NULL}, sizeof(char));
			if (zone_name_type != SQLITE_NULL) {
				body_write(response, zone_name, zone_name_len);
				body_write(response, (char[]){0x00}, sizeof(char));
			}
			body_write(response, (char[]){zone_color_type != SQLITE_NULL}, sizeof(char));
			if (zone_color_type != SQLITE_NULL) {
				body_write(response, zone_color, zone_color_len);
			}
			body_write(response, (char[]){uplink_id_type != SQLITE_NULL}, sizeof(char));
			if (uplink_id_type != SQLITE_NULL) {
				body_write(response, uplink_id, uplink_id_len);
			}
			body_write(response, (char[]){uplink_received_at_type != SQLITE_NULL}, sizeof(char));
			if (uplink_received_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)uplink_received_at)}, sizeof(uplink_received_at));
			}
			*devices_len += 1;
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

int device_parse(device_t *device, request_t *request) {
	request->body.pos = 0;

	uint8_t stage = 0;

	device->name_len = 0;
	const uint8_t name_index = (uint8_t)request->body.pos;
	while (stage == 0 && device->name_len < 16 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 1;
		} else {
			device->name_len++;
		}
	}
	if (device->name_len != 0) {
		device->name = &request->body.ptr[name_index];
	} else {
		device->name = NULL;
	}
	if (stage != 1) {
		debug("found name with %hhu bytes\n", device->name_len);
		return -1;
	}

	if (request->body.len < request->body.pos + sizeof(*device->zone_id)) {
		debug("missing zone id on device\n");
		return -1;
	}
	device->zone_id = (uint8_t (*)[16])body_read(request, sizeof(*device->zone_id));
	stage = 2;

	device->firmware_len = 0;
	const uint8_t firmware_index = (uint8_t)request->body.pos;
	while (stage == 2 && device->firmware_len < 12 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 3;
		} else {
			device->firmware_len++;
		}
	}
	if (device->firmware_len != 0) {
		device->firmware = &request->body.ptr[firmware_index];
	} else {
		device->firmware = NULL;
	}
	if (stage != 3) {
		debug("found firmware with %hhu bytes\n", device->firmware_len);
		return -1;
	}

	device->hardware_len = 0;
	const uint8_t hardware_index = (uint8_t)request->body.pos;
	while (stage == 3 && device->hardware_len < 12 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 4;
		} else {
			device->hardware_len++;
		}
	}
	if (device->hardware_len != 0) {
		device->hardware = &request->body.ptr[hardware_index];
	} else {
		device->hardware = NULL;
	}
	if (stage != 4) {
		debug("found hardware with %hhu bytes\n", device->hardware_len);
		return -1;
	}

	return 0;
}

int device_validate(device_t *device) {
	if (device->name != NULL) {
		if (device->name_len < 2) {
			return -1;
		}

		uint8_t name_index = 0;
		while (name_index < device->name_len) {
			char *byte = &device->name[name_index];
			if ((*byte < 'a' || *byte > 'z') && (*byte < '0' || *byte > '9') && *byte != ' ') {
				debug("name contains invalid character %02x\n", *byte);
				return -1;
			}
			name_index++;
		}
	}

	if (device->firmware != NULL) {
		if (device->firmware_len < 4) {
			return -1;
		}

		uint8_t firmware_index = 0;
		while (firmware_index < device->firmware_len) {
			char *byte = &device->firmware[firmware_index];
			if ((*byte < '0' || *byte > '9') && *byte != '.' && *byte != '-' && *byte != 'r' && *byte != 'c') {
				debug("firmware contains invalid character %02x\n", *byte);
				return -1;
			}
			firmware_index++;
		}
	}

	if (device->hardware != NULL) {
		if (device->hardware_len < 4) {
			return -1;
		}

		uint8_t hardware_index = 0;
		while (hardware_index < device->hardware_len) {
			char *byte = &device->hardware[hardware_index];
			if ((*byte < '0' || *byte > '9') && *byte != '.' && *byte != '-' && *byte != 'r' && *byte != 'c') {
				debug("hardware contains invalid character %02x\n", *byte);
				return -1;
			}
			hardware_index++;
		}
	}

	return 0;
}

uint16_t device_insert(sqlite3 *database, device_t *device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into device (id, name, zone_id, firmware, hardware, created_at) "
										"values (randomblob(16), ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, device->name, device->name_len, SQLITE_STATIC);
	if (device->zone_id != NULL) {
		sqlite3_bind_blob(stmt, 2, device->zone_id, sizeof(*device->zone_id), SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 2);
	}
	if (device->firmware != NULL) {
		sqlite3_bind_text(stmt, 3, device->firmware, device->firmware_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 3);
	}
	if (device->hardware != NULL) {
		sqlite3_bind_text(stmt, 4, device->hardware, device->hardware_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 4);
	}
	sqlite3_bind_int64(stmt, 5, time(NULL));

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
		status = database_error(database, result);
		goto cleanup;
	}

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

uint16_t device_update(sqlite3 *database, device_t *device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "update device "
										"set name = coalesce(?, name), zone_id = coalesce(?, zone_id), "
										"firmware = coalesce(?, firmware), hardware = coalesce(?, hardware), "
										"updated_at = ? "
										"where id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	if (device->name != NULL) {
		sqlite3_bind_text(stmt, 1, device->name, device->name_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 1);
	}
	if (device->zone_id != NULL) {
		sqlite3_bind_blob(stmt, 2, device->zone_id, sizeof(*device->zone_id), SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 2);
	}
	if (device->firmware != NULL) {
		sqlite3_bind_text(stmt, 3, device->firmware, device->firmware_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 3);
	}
	if (device->hardware != NULL) {
		sqlite3_bind_text(stmt, 4, device->hardware, device->hardware_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 4);
	}
	sqlite3_bind_int64(stmt, 5, *device->updated_at);
	sqlite3_bind_blob(stmt, 6, *device->id, sizeof(*device->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_CONSTRAINT) {
		warn("zone %02x%02x not found\n", (*device->zone_id)[0], (*device->zone_id)[1]);
		status = 404;
		goto cleanup;
	}

	if (result != SQLITE_DONE) {
		status = database_error(database, result);
		goto cleanup;
	}

	if (sqlite3_changes(database) == 0) {
		warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
		status = 404;
		goto cleanup;
	}

	status = 0;

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void device_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	device_query_t query;
	if (strnfind(request->search.ptr, request->search.len, "order=", "&", (const char **)&query.order, (size_t *)&query.order_len,
							 16) == -1) {
		response->status = 400;
		return;
	}

	if (strnfind(request->search.ptr, request->search.len, "sort=", "", (const char **)&query.sort, (size_t *)&query.sort_len,
							 8) == -1) {
		response->status = 400;
		return;
	}

	uint8_t devices_len = 0;
	uint16_t status = device_select(database, bwt, &query, response, &devices_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu devices\n", devices_len);
	response->status = 200;
}

void device_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}

void device_find_by_user(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
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

	user_t user = {.id = &id};
	uint16_t status = user_existing(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint8_t devices_len = 0;
	status = device_select_by_user(database, &user, response, &devices_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu devices\n", devices_len);
	response->status = 200;
}

void device_modify(sqlite3 *database, request_t *request, response_t *response) {
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

	time_t now = time(NULL);
	device_t device = {.id = &id, .updated_at = &now};
	if (request->body.len == 0 || device_parse(&device, request) == -1 || device_validate(&device) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = device_update(database, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("updated device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}
