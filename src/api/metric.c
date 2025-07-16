#include "metric.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "../lib/utils.h"
#include "device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *metric_table = "metric";
const char *metric_schema = "create table metric ("
														"id blob primary key, "
														"photovoltaic real not null, "
														"battery real not null, "
														"captured_at timestamp not null, "
														"uplink_id blob not null unique, "
														"device_id blob not null, "
														"foreign key (uplink_id) references uplink(id) on delete cascade, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";

uint16_t metric_select_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, metric_query_t *query, response_t *response,
																 uint16_t *metrics_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"metric.photovoltaic, metric.battery, metric.captured_at "
										"from metric "
										"join user_device on user_device.device_id = metric.device_id and user_device.user_id = ? "
										"where metric.device_id = ? and metric.captured_at >= ? "
										"order by metric.captured_at desc";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, device->id, sizeof(*device->id), SQLITE_STATIC);
	sqlite3_bind_int64(stmt, 3, query->from);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const double photovoltaic = sqlite3_column_double(stmt, 0);
			const double battery = sqlite3_column_double(stmt, 1);
			const time_t captured_at = (time_t)sqlite3_column_int64(stmt, 2);
			append_body(response, (uint16_t[]){hton16((uint16_t)(int16_t)(photovoltaic * 1000))}, sizeof(uint16_t));
			append_body(response, (uint16_t[]){hton16((uint16_t)(battery * 1000))}, sizeof(uint16_t));
			append_body(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
			*metrics_len += 1;
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

uint16_t metric_insert(sqlite3 *database, metric_t *metric) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into metric (id, photovoltaic, battery, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_double(stmt, 1, metric->photovoltaic);
	sqlite3_bind_double(stmt, 2, metric->battery);
	sqlite3_bind_int64(stmt, 3, metric->captured_at);
	sqlite3_bind_blob(stmt, 4, metric->uplink_id, sizeof(*metric->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 5, metric->device_id, sizeof(*metric->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*metric->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*metric->id));
			status = 500;
			goto cleanup;
		}
		memcpy(metric->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("metric is conflicting because %s\n", sqlite3_errmsg(database));
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

void metric_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	const char *from;
	size_t from_len;
	if (strnfind(*request->search, request->search_len, "from=", "", &from, &from_len, 16) == -1) {
		response->status = 400;
		return;
	}

	metric_query_t query = {.from = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1) {
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint16_t metrics_len = 0;
	status = metric_select_by_device(database, bwt, &device, &query, response, &metrics_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	append_header(response, "content-type:application/octet-stream\r\n");
	append_header(response, "content-length:%zu\r\n", response->body_len);
	info("found %hu metrics\n", metrics_len);
	response->status = 200;
}
