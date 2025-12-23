#include "zone.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "database.h"
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

uint16_t zone_existing(sqlite3 *database, bwt_t *bwt, zone_t *zone) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"zone.id, "
										"user_device.device_id "
										"from zone "
										"join device on device.zone_id = zone.id "
										"left join user_device on user_device.device_id = device.id and user_device.user_id = ? "
										"where zone.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, zone->id, sizeof(*zone->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*zone->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*zone->id));
			status = 500;
			goto cleanup;
		}
		const int user_device_device_id_type = sqlite3_column_type(stmt, 1);
		if (user_device_device_id_type == SQLITE_NULL) {
			status = 403;
			goto cleanup;
		}
		memcpy(zone->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("zone %02x%02x not found\n", (*zone->id)[0], (*zone->id)[1]);
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

uint16_t zone_select(sqlite3 *database, bwt_t *bwt, zone_query_t *query, response_t *response, uint8_t *zones_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"zone.id, zone.name, zone.color, "
			"avg(reading.temperature) as reading_temperature, "
			"avg(reading.humidity) as reading_humidity, "
			"max(reading.captured_at), "
			"avg(metric.photovoltaic) as metric_photovoltaic, "
			"avg(metric.battery) as metric_battery, "
			"max(metric.captured_at), "
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
			"case when ?2 = 'level' and ?3 = 'desc' then buffer_level end desc "
			"limit ?4 offset ?5";

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, query->order, (uint8_t)query->order_len, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, query->sort, (uint8_t)query->sort_len, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 4, query->limit);
	sqlite3_bind_int64(stmt, 5, query->offset);

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
			const double reading_temperature = sqlite3_column_double(stmt, 3);
			const int reading_temperature_type = sqlite3_column_type(stmt, 3);
			const double reading_humidity = sqlite3_column_double(stmt, 4);
			const int reading_humidity_type = sqlite3_column_type(stmt, 4);
			const time_t reading_captured_at = (time_t)sqlite3_column_int64(stmt, 5);
			const int reading_captured_at_type = sqlite3_column_type(stmt, 5);
			const double metric_photovoltaic = sqlite3_column_double(stmt, 6);
			const int metric_photovoltaic_type = sqlite3_column_type(stmt, 6);
			const double metric_battery = sqlite3_column_double(stmt, 7);
			const int metric_battery_type = sqlite3_column_type(stmt, 7);
			const time_t metric_captured_at = (time_t)sqlite3_column_int64(stmt, 8);
			const int metric_captured_at_type = sqlite3_column_type(stmt, 8);
			const uint32_t buffer_delay = (uint32_t)sqlite3_column_int(stmt, 9);
			const int buffer_delay_type = sqlite3_column_type(stmt, 9);
			const uint16_t buffer_level = (uint16_t)sqlite3_column_int(stmt, 10);
			const int buffer_level_type = sqlite3_column_type(stmt, 10);
			const time_t buffer_captured_at = (time_t)sqlite3_column_int64(stmt, 11);
			const int buffer_captured_at_type = sqlite3_column_type(stmt, 11);
			body_write(response, id, id_len);
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, color, color_len);
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
			cache_zone_t cache_zone;
			memcpy(cache_zone.id, id, sizeof(cache_zone.id));
			memcpy(cache_zone.name, name, name_len);
			cache_zone.name_len = (uint8_t)name_len;
			if (cache_zone_write(&cache_zone) == -1) {
				warn("failed to cache zone %02x%02x\n", id[0], id[1]);
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

uint16_t zone_select_one(sqlite3 *database, bwt_t *bwt, zone_t *zone, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql =
			"select "
			"zone.id, zone.name, zone.color, zone.created_at, zone.updated_at, "
			"avg(reading.temperature) as reading_temperature, "
			"avg(reading.humidity) as reading_humidity, "
			"max(reading.captured_at), "
			"avg(metric.photovoltaic) as metric_photovoltaic, "
			"avg(metric.battery) as metric_battery, "
			"max(metric.captured_at), "
			"avg(buffer.delay) as buffer_delay, "
			"avg(buffer.level) as buffer_level, "
			"max(buffer.captured_at) "
			"from zone "
			"join device on device.zone_id = zone.id "
			"join user_device on user_device.device_id = device.id and user_device.user_id = ? "
			"left join reading on reading.id = "
			"(select reading.id from reading where reading.device_id = device.id order by reading.captured_at desc limit 1) "
			"left join metric on metric.id = "
			"(select metric.id from metric where metric.device_id = device.id order by metric.captured_at desc limit 1) "
			"left join buffer on buffer.id = "
			"(select buffer.id from buffer where buffer.device_id = device.id order by buffer.captured_at desc limit 1) "
			"where zone.id = ? "
			"group by zone.id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, zone->id, sizeof(*zone->id), SQLITE_STATIC);

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
		const time_t created_at = (time_t)sqlite3_column_int64(stmt, 3);
		const time_t updated_at = (time_t)sqlite3_column_int64(stmt, 4);
		const int updated_at_type = sqlite3_column_type(stmt, 4);
		const double reading_temperature = sqlite3_column_double(stmt, 5);
		const int reading_temperature_type = sqlite3_column_type(stmt, 5);
		const double reading_humidity = sqlite3_column_double(stmt, 6);
		const int reading_humidity_type = sqlite3_column_type(stmt, 6);
		const time_t reading_captured_at = (time_t)sqlite3_column_int64(stmt, 7);
		const int reading_captured_at_type = sqlite3_column_type(stmt, 7);
		const double metric_photovoltaic = sqlite3_column_double(stmt, 8);
		const int metric_photovoltaic_type = sqlite3_column_type(stmt, 8);
		const double metric_battery = sqlite3_column_double(stmt, 9);
		const int metric_battery_type = sqlite3_column_type(stmt, 9);
		const time_t metric_captured_at = (time_t)sqlite3_column_int64(stmt, 10);
		const int metric_captured_at_type = sqlite3_column_type(stmt, 10);
		const uint32_t buffer_delay = (uint32_t)sqlite3_column_int(stmt, 11);
		const int buffer_delay_type = sqlite3_column_type(stmt, 11);
		const uint16_t buffer_level = (uint16_t)sqlite3_column_int(stmt, 12);
		const int buffer_level_type = sqlite3_column_type(stmt, 12);
		const time_t buffer_captured_at = (time_t)sqlite3_column_int64(stmt, 13);
		const int buffer_captured_at_type = sqlite3_column_type(stmt, 13);
		body_write(response, id, id_len);
		body_write(response, name, name_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, color, color_len);
		body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
		body_write(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
		if (updated_at_type != SQLITE_NULL) {
			body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
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
		cache_zone_t cache_zone;
		memcpy(cache_zone.id, id, sizeof(cache_zone.id));
		memcpy(cache_zone.name, name, name_len);
		cache_zone.name_len = (uint8_t)name_len;
		if (cache_zone_write(&cache_zone) == -1) {
			warn("failed to cache zone %02x%02x\n", (*zone->id)[0], (*zone->id)[1]);
		}
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("zone %02x%02x not found\n", (*zone->id)[0], (*zone->id)[1]);
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

int zone_parse(zone_t *zone, request_t *request) {
	request->body.pos = 0;

	uint8_t stage = 0;

	zone->name_len = 0;
	const uint8_t name_index = (uint8_t)request->body.pos;
	while (stage == 0 && zone->name_len < 12 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 1;
		} else {
			zone->name_len++;
		}
	}
	if (zone->name_len != 0) {
		zone->name = &request->body.ptr[name_index];
	} else {
		zone->name = NULL;
	}
	if (stage != 1) {
		debug("found name with %hhu bytes\n", zone->name_len);
		return -1;
	}

	if (request->body.len < request->body.pos + sizeof(*zone->color)) {
		debug("missing color on zone\n");
		return -1;
	}
	zone->color = (uint8_t (*)[12])body_read(request, sizeof(*zone->color));
	stage = 2;

	return 0;
}

int zone_validate(zone_t *zone) {
	if (zone->name_len < 2) {
		return -1;
	}

	uint8_t name_index = 0;
	while (name_index < zone->name_len) {
		char *byte = &zone->name[name_index];
		if ((*byte < 'a' || *byte > 'z') && (*byte < '0' || *byte > '9')) {
			debug("name contains invalid character %02x\n", *byte);
			return -1;
		}
		name_index++;
	}

	return 0;
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

uint16_t zone_update(sqlite3 *database, zone_t *zone) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "update zone "
										"set name = ?, "
										"color = ?, "
										"updated_at = ? "
										"where id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_text(stmt, 1, zone->name, zone->name_len, SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, *zone->color, sizeof(*zone->color), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, *zone->updated_at);
	sqlite3_bind_blob(stmt, 4, *zone->id, sizeof(*zone->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result != SQLITE_DONE) {
		status = database_error(database, result);
		goto cleanup;
	}

	if (sqlite3_changes(database) == 0) {
		warn("zone %02x%02x not found\n", (*zone->id)[0], (*zone->id)[1]);
		status = 404;
		goto cleanup;
	}

	status = 0;

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void zone_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	zone_query_t query = {.limit = 16, .offset = 0};
	if (strnfind(request->search.ptr, request->search.len, "order=", "&", &query.order, &query.order_len, 16) == -1) {
		response->status = 400;
		return;
	}

	if (strnfind(request->search.ptr, request->search.len, "sort=", "", &query.sort, &query.sort_len, 8) == -1) {
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

void zone_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((zone_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((zone_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	zone_t zone = {.id = &id};
	uint16_t status = zone_existing(database, bwt, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = zone_select_one(database, bwt, &zone, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	cache_zone_t cache_zone;
	if (cache_zone_read(&cache_zone, &zone) != -1) {
		header_write(response, "zone-name:%.*s\r\n", cache_zone.name_len, cache_zone.name);
	}
	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found zone %02x%02x\n", (*zone.id)[0], (*zone.id)[1]);
	response->status = 200;
}

void zone_modify(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((zone_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((zone_t *)0)->id) * 2);
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
	zone_t zone = {.id = &id, .updated_at = &now};
	if (request->body.len == 0 || zone_parse(&zone, request) == -1 || zone_validate(&zone) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = zone_update(database, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("updated zone %02x%02x\n", (*zone.id)[0], (*zone.id)[1]);
	response->status = 200;
}
