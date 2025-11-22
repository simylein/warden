#include "zone.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "buffer.h"
#include "database.h"
#include "metric.h"
#include "reading.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *zone_table = "zone";
const char *zone_schema = "create table zone ("
													"id blob primary key, "
													"name text not null unique, "
													"color blob not null, "
													"created_at timestamp not null, "
													"updated_at timestamp"
													")";

uint16_t zone_select(sqlite3 *database, bwt_t *bwt, zone_query_t *query, response_t *response, uint8_t *zones_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"zone.id, zone.name, zone.color, "
			"reading.id, "
			"avg(reading.temperature) as reading_temperature, "
			"avg(reading.humidity) as reading_humidity, "
			"max(reading.captured_at), "
			"metric.id, "
			"avg(metric.photovoltaic) as metric_photovoltaic, "
			"avg(metric.battery) as metric_battery, "
			"max(metric.captured_at), "
			"buffer.id, "
			"avg(buffer.delay) as buffer_delay, "
			"avg(buffer.level) as buffer_level, "
			"max(buffer.captured_at) "
			"from zone "
			"join device on device.zone_id = zone.id "
			"join user_device on user_device.device_id = device.id and user_device.user_id = ?1 "
			"left join reading on reading.id = "
			"(select reading.id from reading where reading.device_id = device.id order by reading.captured_at desc limit 1) "
			"left join metric on metric.id = "
			"(select metric.id from metric where metric.device_id = device.id order by metric.captured_at desc limit 1) "
			"left join buffer on buffer.id = "
			"(select buffer.id from buffer where buffer.device_id = device.id order by buffer.captured_at desc limit 1) "
			"group by zone.id "
			"order by "
			"case when ?2 = 'id' and ?3 = 'asc' then zone.id end asc, "
			"case when ?2 = 'id' and ?3 = 'desc' then zone.id end desc, "
			"case when ?2 = 'name' and ?3 = 'asc' then zone.name end asc, "
			"case when ?2 = 'name' and ?3 = 'desc' then zone.name end desc, "
			"case when ?2 = 'temperature' and ?3 = 'asc' then reading_temperature end asc, "
			"case when ?2 = 'temperature' and ?3 = 'desc' then reading_temperature end desc, "
			"case when ?2 = 'humidity' and ?3 = 'asc' then reading_humidity end asc, "
			"case when ?2 = 'humidity' and ?3 = 'desc' then reading_humidity end desc, "
			"case when ?2 = 'photovoltaic' and ?3 = 'asc' then metric_photovoltaic end asc, "
			"case when ?2 = 'photovoltaic' and ?3 = 'desc' then metric_photovoltaic end desc, "
			"case when ?2 = 'battery' and ?3 = 'asc' then metric_battery end asc, "
			"case when ?2 = 'battery' and ?3 = 'desc' then metric_battery end desc, "
			"case when ?2 = 'delay' and ?3 = 'asc' then buffer_delay end asc, "
			"case when ?2 = 'delay' and ?3 = 'desc' then buffer_delay end desc, "
			"case when ?2 = 'level' and ?3 = 'asc' then buffer_level end asc, "
			"case when ?2 = 'level' and ?3 = 'desc' then buffer_level end desc";

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
			if (id_len != sizeof(*((zone_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((zone_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *name = sqlite3_column_text(stmt, 1);
			const size_t name_len = (size_t)sqlite3_column_bytes(stmt, 1);
			const uint8_t *color = sqlite3_column_blob(stmt, 2);
			const size_t color_len = (size_t)sqlite3_column_bytes(stmt, 2);
			if (color_len != sizeof(*((zone_t *)0)->color)) {
				error("color length %zu does not match buffer length %zu\n", color_len, sizeof(*((zone_t *)0)->color));
				status = 500;
				goto cleanup;
			}
			const uint8_t *reading_id = sqlite3_column_blob(stmt, 3);
			const size_t reading_id_len = (size_t)sqlite3_column_bytes(stmt, 3);
			const int reading_id_type = sqlite3_column_type(stmt, 3);
			if (reading_id_type != SQLITE_NULL && reading_id_len != sizeof(*((reading_t *)0)->id)) {
				error("reading id length %zu does not match buffer length %zu\n", reading_id_len, sizeof(*((reading_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const double reading_temperature = sqlite3_column_double(stmt, 4);
			const int reading_temperature_type = sqlite3_column_type(stmt, 4);
			const double reading_humidity = sqlite3_column_double(stmt, 5);
			const int reading_humidity_type = sqlite3_column_type(stmt, 5);
			const time_t reading_captured_at = (time_t)sqlite3_column_int64(stmt, 6);
			const int reading_captured_at_type = sqlite3_column_type(stmt, 6);
			const uint8_t *metric_id = sqlite3_column_blob(stmt, 7);
			const size_t metric_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
			const int metric_id_type = sqlite3_column_type(stmt, 7);
			if (metric_id_type != SQLITE_NULL && metric_id_len != sizeof(*((metric_t *)0)->id)) {
				error("metric id length %zu does not match buffer length %zu\n", metric_id_len, sizeof(*((metric_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const double metric_photovoltaic = sqlite3_column_double(stmt, 8);
			const int metric_photovoltaic_type = sqlite3_column_type(stmt, 8);
			const double metric_battery = sqlite3_column_double(stmt, 9);
			const int metric_battery_type = sqlite3_column_type(stmt, 9);
			const time_t metric_captured_at = (time_t)sqlite3_column_int64(stmt, 10);
			const int metric_captured_at_type = sqlite3_column_type(stmt, 10);
			const uint8_t *buffer_id = sqlite3_column_blob(stmt, 11);
			const size_t buffer_id_len = (size_t)sqlite3_column_bytes(stmt, 11);
			const int buffer_id_type = sqlite3_column_type(stmt, 11);
			if (buffer_id_type != SQLITE_NULL && buffer_id_len != sizeof(*((buffer_t *)0)->id)) {
				error("buffer id length %zu does not match buffer length %zu\n", buffer_id_len, sizeof(*((buffer_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint32_t buffer_delay = (uint32_t)sqlite3_column_int(stmt, 12);
			const int buffer_delay_type = sqlite3_column_type(stmt, 12);
			const uint16_t buffer_level = (uint16_t)sqlite3_column_int(stmt, 13);
			const int buffer_level_type = sqlite3_column_type(stmt, 13);
			const time_t buffer_captured_at = (time_t)sqlite3_column_int64(stmt, 14);
			const int buffer_captured_at_type = sqlite3_column_type(stmt, 14);
			body_write(response, id, id_len);
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, color, color_len);
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
			*zones_len += 1;
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

uint16_t zone_insert(sqlite3 *database, zone_t *zone) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into zone (id, name, color, created_at) "
										"values (randomblob(16), ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, zone->name, zone->name_len, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, zone->color, sizeof(*zone->color), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, time(NULL));

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*zone->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*zone->id));
			status = 500;
			goto cleanup;
		}
		memcpy(zone->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("zone name %.*s already taken\n", (int)zone->name_len, zone->name);
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

void zone_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	zone_query_t query;
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

	uint8_t zones_len = 0;
	uint16_t status = zone_select(database, bwt, &query, response, &zones_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu zones\n", zones_len);
	response->status = 200;
}
