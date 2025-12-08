#include "reading.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "cache.h"
#include "database.h"
#include "device.h"
#include "time.h"
#include "zone.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *reading_table = "reading";
const char *reading_schema = "create table reading ("
														 "id blob primary key, "
														 "temperature real not null, "
														 "humidity real not null, "
														 "captured_at timestamp not null, "
														 "uplink_id blob not null unique, "
														 "device_id blob not null, "
														 "foreign key (uplink_id) references uplink(id) on delete cascade, "
														 "foreign key (device_id) references device(id) on delete cascade"
														 ")";

uint16_t reading_select(sqlite3 *database, bwt_t *bwt, reading_query_t *query, response_t *response, uint16_t *readings_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"avg(reading.temperature), avg(reading.humidity), "
										"(reading.captured_at / ?) * ? as bucket_time, "
										"user_device.device_id "
										"from reading "
										"join user_device on user_device.device_id = reading.device_id and user_device.user_id = ? "
										"where reading.captured_at >= ? and reading.captured_at <= ? "
										"group by bucket_time, user_device.device_id "
										"order by bucket_time desc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, query->bucket);
	sqlite3_bind_int(stmt, 2, query->bucket);
	sqlite3_bind_blob(stmt, 3, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 4, query->from);
	sqlite3_bind_int64(stmt, 5, query->to);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const double temperature = sqlite3_column_double(stmt, 0);
			const double humidity = sqlite3_column_double(stmt, 1);
			const time_t captured_at = (time_t)sqlite3_column_int64(stmt, 2);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 3);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 3);
			if (device_id_len != sizeof(*((device_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((device_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			if (response->body.len + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(captured_at) + device_id_len > response->body.cap) {
				error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
				status = 500;
				goto cleanup;
			}
			body_write(response, (uint16_t[]){hton16((uint16_t)(int16_t)(temperature * 100))}, sizeof(uint16_t));
			body_write(response, (uint16_t[]){hton16((uint16_t)(humidity * 100))}, sizeof(uint16_t));
			body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
			body_write(response, device_id, device_id_len);
			*readings_len += 1;
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

uint16_t reading_select_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, reading_query_t *query, response_t *response,
																	uint16_t *readings_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"avg(reading.temperature), avg(reading.humidity), "
										"(reading.captured_at / ?) * ? as bucket_time "
										"from reading "
										"join user_device on user_device.device_id = reading.device_id and user_device.user_id = ? "
										"where reading.device_id = ? and reading.captured_at >= ? and reading.captured_at <= ? "
										"group by bucket_time "
										"order by bucket_time desc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, query->bucket);
	sqlite3_bind_int(stmt, 2, query->bucket);
	sqlite3_bind_blob(stmt, 3, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 4, device->id, sizeof(*device->id), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 5, query->from);
	sqlite3_bind_int64(stmt, 6, query->to);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const double temperature = sqlite3_column_double(stmt, 0);
			const double humidity = sqlite3_column_double(stmt, 1);
			const time_t captured_at = (time_t)sqlite3_column_int64(stmt, 2);
			body_write(response, (uint16_t[]){hton16((uint16_t)(int16_t)(temperature * 100))}, sizeof(uint16_t));
			body_write(response, (uint16_t[]){hton16((uint16_t)(humidity * 100))}, sizeof(uint16_t));
			body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
			*readings_len += 1;
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

uint16_t reading_select_by_zone(sqlite3 *database, bwt_t *bwt, zone_t *zone, reading_query_t *query, response_t *response,
																uint16_t *readings_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"avg(reading.temperature), avg(reading.humidity), "
										"(reading.captured_at / ?) * ? as bucket_time, "
										"user_device.device_id "
										"from reading "
										"join user_device on user_device.device_id = reading.device_id and user_device.user_id = ? "
										"join device on device.id = reading.device_id "
										"where device.zone_id = ? and reading.captured_at >= ? and reading.captured_at <= ? "
										"group by bucket_time, user_device.device_id "
										"order by bucket_time desc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, query->bucket);
	sqlite3_bind_int(stmt, 2, query->bucket);
	sqlite3_bind_blob(stmt, 3, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 4, zone->id, sizeof(*zone->id), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 5, query->from);
	sqlite3_bind_int64(stmt, 6, query->to);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const double temperature = sqlite3_column_double(stmt, 0);
			const double humidity = sqlite3_column_double(stmt, 1);
			const time_t captured_at = (time_t)sqlite3_column_int64(stmt, 2);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 3);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 3);
			if (device_id_len != sizeof(*((device_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((device_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			if (response->body.len + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(captured_at) + device_id_len > response->body.cap) {
				error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
				status = 500;
				goto cleanup;
			}
			body_write(response, (uint16_t[]){hton16((uint16_t)(int16_t)(temperature * 100))}, sizeof(uint16_t));
			body_write(response, (uint16_t[]){hton16((uint16_t)(humidity * 100))}, sizeof(uint16_t));
			body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
			body_write(response, device_id, device_id_len);
			*readings_len += 1;
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

uint16_t reading_insert(sqlite3 *database, reading_t *reading) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into reading (id, temperature, humidity, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_double(stmt, 1, reading->temperature);
	sqlite3_bind_double(stmt, 2, reading->humidity);
	sqlite3_bind_int64(stmt, 3, reading->captured_at);
	sqlite3_bind_blob(stmt, 4, reading->uplink_id, sizeof(*reading->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 5, reading->device_id, sizeof(*reading->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*reading->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*reading->id));
			status = 500;
			goto cleanup;
		}
		memcpy(reading->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("reading is conflicting because %s\n", sqlite3_errmsg(database));
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

void reading_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	const char *from;
	size_t from_len;
	if (strnfind(request->search.ptr, request->search.len, "from=", "&", &from, &from_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *to;
	size_t to_len;
	if (strnfind(request->search.ptr, request->search.len, "to=", "", &to, &to_len, 32) == -1) {
		response->status = 400;
		return;
	}

	reading_query_t query = {.from = 0, .to = 0, .bucket = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1 || strnto64(to, to_len, (uint64_t *)&query.to) == -1) {
		warn("failed to parse query from %*.s to %*.s\n", (int)from_len, from, (int)to_len, to);
		response->status = 400;
		return;
	}

	if (query.from > query.to || query.to - query.from < 3600 || query.to - query.from > 1209600) {
		warn("failed to validate query from %lu to %lu\n", query.from, query.to);
		response->status = 400;
		return;
	}

	time_t range = query.to - query.from;
	if (range <= 3600) {
		query.bucket = 5;
	} else if (range <= 10800) {
		query.bucket = 15;
	} else if (range <= 43200) {
		query.bucket = 60;
	} else if (range <= 86400) {
		query.bucket = 120;
	} else if (range <= 172800) {
		query.bucket = 240;
	} else if (range <= 345600) {
		query.bucket = 480;
	} else if (range <= 604800) {
		query.bucket = 840;
	} else if (range <= 1209600) {
		query.bucket = 1680;
	}

	uint16_t readings_len = 0;
	uint16_t status = reading_select(database, bwt, &query, response, &readings_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hu readings\n", readings_len);
	response->status = 200;
}

void reading_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	const char *from;
	size_t from_len;
	if (strnfind(request->search.ptr, request->search.len, "from=", "&", &from, &from_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *to;
	size_t to_len;
	if (strnfind(request->search.ptr, request->search.len, "to=", "", &to, &to_len, 32) == -1) {
		response->status = 400;
		return;
	}

	reading_query_t query = {.from = 0, .to = 0, .bucket = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1 || strnto64(to, to_len, (uint64_t *)&query.to) == -1) {
		warn("failed to parse query from %*.s to %*.s\n", (int)from_len, from, (int)to_len, to);
		response->status = 400;
		return;
	}

	if (query.from > query.to || query.to - query.from < 3600 || query.to - query.from > 1209600) {
		warn("failed to validate query from %lu to %lu\n", query.from, query.to);
		response->status = 400;
		return;
	}

	time_t range = query.to - query.from;
	if (range <= 3600) {
		query.bucket = 5;
	} else if (range <= 10800) {
		query.bucket = 15;
	} else if (range <= 43200) {
		query.bucket = 60;
	} else if (range <= 86400) {
		query.bucket = 120;
	} else if (range <= 172800) {
		query.bucket = 240;
	} else if (range <= 345600) {
		query.bucket = 480;
	} else if (range <= 604800) {
		query.bucket = 840;
	} else if (range <= 1209600) {
		query.bucket = 1680;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint16_t readings_len = 0;
	status = reading_select_by_device(database, bwt, &device, &query, response, &readings_len);
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
	info("found %hu readings\n", readings_len);
	response->status = 200;
}

void reading_find_by_zone(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	const char *from;
	size_t from_len;
	if (strnfind(request->search.ptr, request->search.len, "from=", "&", &from, &from_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *to;
	size_t to_len;
	if (strnfind(request->search.ptr, request->search.len, "to=", "", &to, &to_len, 32) == -1) {
		response->status = 400;
		return;
	}

	reading_query_t query = {.from = 0, .to = 0, .bucket = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1 || strnto64(to, to_len, (uint64_t *)&query.to) == -1) {
		warn("failed to parse query from %*.s to %*.s\n", (int)from_len, from, (int)to_len, to);
		response->status = 400;
		return;
	}

	if (query.from > query.to || query.to - query.from < 3600 || query.to - query.from > 1209600) {
		warn("failed to validate query from %lu to %lu\n", query.from, query.to);
		response->status = 400;
		return;
	}

	time_t range = query.to - query.from;
	if (range <= 3600) {
		query.bucket = 5;
	} else if (range <= 10800) {
		query.bucket = 15;
	} else if (range <= 43200) {
		query.bucket = 60;
	} else if (range <= 86400) {
		query.bucket = 120;
	} else if (range <= 172800) {
		query.bucket = 240;
	} else if (range <= 345600) {
		query.bucket = 480;
	} else if (range <= 604800) {
		query.bucket = 840;
	} else if (range <= 1209600) {
		query.bucket = 1680;
	}

	zone_t zone = {.id = &id};
	uint16_t status = zone_existing(database, bwt, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint16_t readings_len = 0;
	status = reading_select_by_zone(database, bwt, &zone, &query, response, &readings_len);
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
	info("found %hu readings\n", readings_len);
	response->status = 200;
}
