#include "uplink.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "cache.h"
#include "database.h"
#include "decode.h"
#include "device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const char *uplink_table = "uplink";
const char *uplink_schema = "create table uplink ("
														"id blob primary key, "
														"kind integer not null, "
														"data blob not null, "
														"airtime real not null, "
														"frequency integer not null, "
														"bandwidth integer not null, "
														"rssi integer not null, "
														"snr real not null, "
														"sf integer not null, "
														"cr integer not null, "
														"tx_power integer not null, "
														"preamble_len integer not null, "
														"received_at timestamp not null, "
														"device_id blob not null, "
														"foreign key (device_id) references device(id) on delete cascade"
														")";

uint16_t uplink_existing(sqlite3 *database, bwt_t *bwt, uplink_t *uplink) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"uplink.id, "
										"user_device.device_id "
										"from uplink "
										"left join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"where uplink.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, uplink->id, sizeof(*uplink->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*uplink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*uplink->id));
			status = 500;
			goto cleanup;
		}
		const int user_device_device_id_type = sqlite3_column_type(stmt, 1);
		if (user_device_device_id_type == SQLITE_NULL) {
			status = 403;
			goto cleanup;
		}
		memcpy(uplink->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("uplink %02x%02x not found\n", (*uplink->id)[0], (*uplink->id)[1]);
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

uint16_t uplink_select(sqlite3 *database, bwt_t *bwt, uplink_query_t *query, response_t *response, uint8_t *uplinks_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, kind, data, rssi, snr, sf, tx_power, received_at, uplink.device_id from uplink "
										"join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"order by received_at desc "
										"limit ? offset ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_int(stmt, 2, query->limit);
	sqlite3_bind_int(stmt, 3, (int)query->offset);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const uint8_t *id = sqlite3_column_blob(stmt, 0);
			const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
			if (id_len != sizeof(*((uplink_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t kind = (uint8_t)sqlite3_column_int(stmt, 1);
			const uint8_t *data = sqlite3_column_blob(stmt, 2);
			const size_t data_len = (size_t)sqlite3_column_bytes(stmt, 2);
			if (data_len > UINT8_MAX) {
				error("data length %zu exceeds buffer length %hhu\n", data_len, UINT8_MAX);
				status = 500;
				goto cleanup;
			}
			const int16_t rssi = (int16_t)sqlite3_column_int(stmt, 3);
			const double snr = sqlite3_column_double(stmt, 4);
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 5);
			const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 6);
			const time_t received_at = (time_t)sqlite3_column_int64(stmt, 7);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 8);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 8);
			if (device_id_len != sizeof(*((uplink_t *)0)->device_id)) {
				error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((uplink_t *)0)->device_id));
				status = 500;
				goto cleanup;
			}
			body_write(response, id, id_len);
			body_write(response, &kind, sizeof(kind));
			body_write(response, &data_len, sizeof(uint8_t));
			body_write(response, data, data_len);
			body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
			body_write(response, (uint8_t[]){(uint8_t)(int8_t)(snr * 4)}, sizeof(uint8_t));
			body_write(response, &sf, sizeof(sf));
			body_write(response, &tx_power, sizeof(tx_power));
			body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
			body_write(response, device_id, device_id_len);
			*uplinks_len += 1;
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

uint16_t uplink_select_one(sqlite3 *database, bwt_t *bwt, uplink_t *uplink, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"uplink.id, uplink.kind, uplink.data, "
										"uplink.airtime, uplink.frequency, uplink.bandwidth, "
										"uplink.rssi, uplink.snr, uplink.sf, uplink.cr, uplink.tx_power, uplink.preamble_len, "
										"uplink.received_at, uplink.device_id "
										"from uplink "
										"join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"where uplink.id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, uplink->id, sizeof(*uplink->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*uplink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*uplink->id));
			status = 500;
			goto cleanup;
		}
		const uint8_t kind = (uint8_t)sqlite3_column_int(stmt, 1);
		const uint8_t *data = sqlite3_column_blob(stmt, 2);
		const size_t data_len = (size_t)sqlite3_column_bytes(stmt, 2);
		if (data_len > UINT8_MAX) {
			error("data length %zu exceeds buffer length %hhu\n", id_len, UINT8_MAX);
			status = 500;
			goto cleanup;
		}
		const double airtime = sqlite3_column_double(stmt, 3);
		const uint32_t frequency = (uint32_t)sqlite3_column_int(stmt, 4);
		const uint32_t bandwidth = (uint32_t)sqlite3_column_int(stmt, 5);
		const int16_t rssi = (int16_t)sqlite3_column_int(stmt, 6);
		const double snr = sqlite3_column_double(stmt, 7);
		const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 8);
		const uint8_t cr = (uint8_t)sqlite3_column_int(stmt, 9);
		const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 10);
		const uint8_t preamble_len = (uint8_t)sqlite3_column_int(stmt, 11);
		const time_t received_at = (time_t)sqlite3_column_int64(stmt, 12);
		const uint8_t *device_id = sqlite3_column_blob(stmt, 13);
		const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 13);
		if (device_id_len != sizeof(*((uplink_t *)0)->device_id)) {
			error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((uplink_t *)0)->device_id));
			status = 500;
			goto cleanup;
		}
		body_write(response, id, id_len);
		body_write(response, &kind, sizeof(kind));
		body_write(response, &data_len, sizeof(uint8_t));
		body_write(response, data, data_len);
		body_write(response, (uint16_t[]){hton16((uint16_t)(airtime * 16 * 1000))}, sizeof(uint16_t));
		body_write(response, (uint32_t[]){hton32(frequency)}, sizeof(frequency));
		body_write(response, (uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
		body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
		body_write(response, (uint8_t[]){(uint8_t)(int8_t)(snr * 4)}, sizeof(uint8_t));
		body_write(response, &sf, sizeof(sf));
		body_write(response, &cr, sizeof(cr));
		body_write(response, &tx_power, sizeof(tx_power));
		body_write(response, &preamble_len, sizeof(preamble_len));
		body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
		body_write(response, device_id, device_id_len);
		status = 0;
	} else if (result == SQLITE_DONE) {
		warn("uplink %02x%02x not found\n", (*uplink->id)[0], (*uplink->id)[1]);
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

uint16_t uplink_select_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, uplink_query_t *query, response_t *response,
																 uint16_t *uplinks_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select id, kind, data, rssi, snr, sf, tx_power, received_at from uplink "
										"join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"where uplink.device_id = ? "
										"order by received_at desc "
										"limit ? offset ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, device->id, sizeof(*device->id), SQLITE_STATIC);
	sqlite3_bind_int(stmt, 3, query->limit);
	sqlite3_bind_int(stmt, 4, (int)query->offset);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const uint8_t *id = sqlite3_column_blob(stmt, 0);
			const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
			if (id_len != sizeof(*((uplink_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t kind = (uint8_t)sqlite3_column_int(stmt, 1);
			const uint8_t *data = sqlite3_column_blob(stmt, 2);
			const size_t data_len = (size_t)sqlite3_column_bytes(stmt, 2);
			if (data_len > UINT8_MAX) {
				error("data length %zu exceeds buffer length %hhu\n", data_len, UINT8_MAX);
				status = 500;
				goto cleanup;
			}
			const int16_t rssi = (int16_t)sqlite3_column_int(stmt, 3);
			const double snr = sqlite3_column_double(stmt, 4);
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 5);
			const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 6);
			const time_t received_at = (time_t)sqlite3_column_int64(stmt, 7);
			body_write(response, id, id_len);
			body_write(response, &kind, sizeof(kind));
			body_write(response, &data_len, sizeof(uint8_t));
			body_write(response, data, data_len);
			body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
			body_write(response, (uint8_t[]){(uint8_t)(int8_t)(snr * 4)}, sizeof(uint8_t));
			body_write(response, &sf, sizeof(sf));
			body_write(response, &tx_power, sizeof(tx_power));
			body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
			*uplinks_len += 1;
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

uint16_t uplink_signal_select_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, uplink_signal_query_t *query,
																				response_t *response, uint16_t *signals_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"avg(uplink.rssi), avg(uplink.snr), avg(uplink.sf), "
										"(uplink.received_at / ?) * ? as bucket_time "
										"from uplink "
										"join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"where uplink.device_id = ? and uplink.received_at >= ? and uplink.received_at <= ? "
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
			const int16_t rssi = (int16_t)sqlite3_column_int(stmt, 0);
			const double snr = sqlite3_column_double(stmt, 1);
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 2);
			const time_t received_at = (time_t)sqlite3_column_int64(stmt, 3);
			body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
			body_write(response, (uint8_t[]){(uint8_t)(int8_t)(snr * 4)}, sizeof(uint8_t));
			body_write(response, &sf, sizeof(sf));
			body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
			*signals_len += 1;
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

uint16_t uplink_signal_select_by_zone(sqlite3 *database, bwt_t *bwt, zone_t *zone, uplink_signal_query_t *query,
																			response_t *response, uint16_t *signals_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"avg(uplink.rssi), avg(uplink.snr), avg(uplink.sf), "
										"(uplink.received_at / ?) * ? as bucket_time, uplink.device_id "
										"from uplink "
										"join user_device on user_device.device_id = uplink.device_id and user_device.user_id = ? "
										"join device on device.id = uplink.device_id "
										"where device.zone_id = ? and uplink.received_at >= ? and uplink.received_at <= ? "
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
			const int16_t rssi = (int16_t)sqlite3_column_int(stmt, 0);
			const double snr = sqlite3_column_double(stmt, 1);
			const uint8_t sf = (uint8_t)sqlite3_column_int(stmt, 2);
			const time_t received_at = (time_t)sqlite3_column_int64(stmt, 3);
			const uint8_t *device_id = sqlite3_column_blob(stmt, 4);
			const size_t device_id_len = (size_t)sqlite3_column_bytes(stmt, 4);
			if (device_id_len != sizeof(*((uplink_t *)0)->device_id)) {
				error("device id length %zu does not match buffer length %zu\n", device_id_len, sizeof(*((uplink_t *)0)->device_id));
				status = 500;
				goto cleanup;
			}
			body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
			body_write(response, (uint8_t[]){(uint8_t)(int8_t)(snr * 4)}, sizeof(uint8_t));
			body_write(response, &sf, sizeof(sf));
			body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
			body_write(response, device_id, device_id_len);
			*signals_len += 1;
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

int uplink_parse(uplink_t *uplink, request_t *request) {
	request->body.pos = 0;

	if (request->body.len < request->body.pos + sizeof(uplink->kind)) {
		debug("missing kind on uplink\n");
		return -1;
	}
	uplink->kind = *(uint8_t *)body_read(request, sizeof(uplink->kind));

	if (request->body.len < request->body.pos + sizeof(uplink->data_len)) {
		debug("missing data len on uplink\n");
		return -1;
	}
	uplink->data_len = *(uint8_t *)body_read(request, sizeof(uplink->data_len));

	if (request->body.len < request->body.pos + uplink->data_len) {
		debug("missing data on uplink\n");
		return -1;
	}
	uplink->data = (uint8_t *)body_read(request, uplink->data_len);

	if (request->body.len < request->body.pos + sizeof(uplink->airtime)) {
		debug("missing airtime on uplink\n");
		return -1;
	}
	memcpy(&uplink->airtime, body_read(request, sizeof(uplink->airtime)), sizeof(uplink->airtime));
	uplink->airtime = ntoh16(uplink->airtime);

	if (request->body.len < request->body.pos + sizeof(uplink->frequency)) {
		debug("missing frequency on uplink\n");
		return -1;
	}
	memcpy(&uplink->frequency, body_read(request, sizeof(uplink->frequency)), sizeof(uplink->frequency));
	uplink->frequency = ntoh32(uplink->frequency);

	if (request->body.len < request->body.pos + sizeof(uplink->bandwidth)) {
		debug("missing bandwidth on uplink\n");
		return -1;
	}
	memcpy(&uplink->bandwidth, body_read(request, sizeof(uplink->bandwidth)), sizeof(uplink->bandwidth));
	uplink->bandwidth = ntoh32(uplink->bandwidth);

	if (request->body.len < request->body.pos + sizeof(uplink->rssi)) {
		debug("missing rssi on uplink\n");
		return -1;
	}
	memcpy(&uplink->rssi, body_read(request, sizeof(uplink->rssi)), sizeof(uplink->rssi));
	uplink->rssi = (int16_t)ntoh16((uint16_t)uplink->rssi);

	if (request->body.len < request->body.pos + sizeof(uplink->snr)) {
		debug("missing snr on uplink\n");
		return -1;
	}
	uplink->snr = *(int8_t *)body_read(request, sizeof(uplink->snr));

	if (request->body.len < request->body.pos + sizeof(uplink->sf)) {
		debug("missing sf on uplink\n");
		return -1;
	}
	uplink->sf = *(uint8_t *)body_read(request, sizeof(uplink->sf));

	if (request->body.len < request->body.pos + sizeof(uplink->cr)) {
		debug("missing cr on uplink\n");
		return -1;
	}
	uplink->cr = *(uint8_t *)body_read(request, sizeof(uplink->cr));

	if (request->body.len < request->body.pos + sizeof(uplink->tx_power)) {
		debug("missing tx power on uplink\n");
		return -1;
	}
	uplink->tx_power = *(uint8_t *)body_read(request, sizeof(uplink->tx_power));

	if (request->body.len < request->body.pos + sizeof(uplink->preamble_len)) {
		debug("missing preamble len on uplink\n");
		return -1;
	}
	uplink->preamble_len = *(uint8_t *)body_read(request, sizeof(uplink->preamble_len));

	if (request->body.len < request->body.pos + sizeof(uplink->received_at)) {
		debug("missing received at on uplink\n");
		return -1;
	}
	memcpy(&uplink->received_at, body_read(request, sizeof(uplink->received_at)), sizeof(uplink->received_at));
	uplink->received_at = (time_t)ntoh64((uint64_t)uplink->received_at);

	if (request->body.len < request->body.pos + sizeof(*uplink->device_id)) {
		debug("missing device id on uplink\n");
		return -1;
	}
	uplink->device_id = (uint8_t (*)[16])body_read(request, sizeof(*uplink->device_id));

	if (request->body.len != request->body.pos) {
		debug("body len %u does not match body pos %u\n", request->body.len, request->body.pos);
		return -1;
	}

	return 0;
}

int uplink_validate(uplink_t *uplink) {
	if (uplink->airtime < 128) {
		debug("invalid airtime %hu on uplink\n", uplink->airtime);
		return -1;
	}

	if (uplink->frequency < 400 * 1000 * 1000 || uplink->frequency > 500 * 1000 * 1000) {
		debug("invalid frequency %u on uplink\n", uplink->frequency);
		return -1;
	}

	if (uplink->bandwidth < 7800 || uplink->bandwidth > 500 * 1000) {
		debug("invalid bandwidth %u on uplink\n", uplink->bandwidth);
		return -1;
	}

	if (uplink->rssi < -192 || uplink->rssi > 16) {
		debug("invalid rssi %hd on uplink\n", uplink->rssi);
		return -1;
	}

	if (uplink->snr < -112 || uplink->snr > 56) {
		debug("invalid snr %hhd on uplink\n", uplink->snr);
		return -1;
	}

	if (uplink->sf < 6 || uplink->sf > 12) {
		debug("invalid sf %hhu on uplink\n", uplink->sf);
		return -1;
	}

	if (uplink->cr < 5 || uplink->cr > 8) {
		debug("invalid cr %hhu on uplink\n", uplink->cr);
		return -1;
	}

	if (uplink->tx_power < 2 || uplink->tx_power > 17) {
		debug("invalid tx power %hhu on uplink\n", uplink->tx_power);
		return -1;
	}

	if (uplink->preamble_len < 6 || uplink->preamble_len > 21) {
		debug("invalid preamble len %hhu on uplink\n", uplink->preamble_len);
		return -1;
	}

	return 0;
}

uint16_t uplink_insert(sqlite3 *database, uplink_t *uplink) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into uplink (id, kind, data, airtime, frequency, bandwidth, "
										"rssi, snr, sf, cr, tx_power, preamble_len, received_at, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, uplink->kind);
	sqlite3_bind_blob(stmt, 2, uplink->data, uplink->data_len, SQLITE_STATIC);
	sqlite3_bind_double(stmt, 3, (double)uplink->airtime / (16 * 1000));
	sqlite3_bind_int(stmt, 4, (int)uplink->frequency);
	sqlite3_bind_int(stmt, 5, (int)uplink->bandwidth);
	sqlite3_bind_int(stmt, 6, uplink->rssi);
	sqlite3_bind_double(stmt, 7, (double)uplink->snr / 4);
	sqlite3_bind_int(stmt, 8, uplink->sf);
	sqlite3_bind_int(stmt, 9, uplink->cr);
	sqlite3_bind_int(stmt, 10, uplink->tx_power);
	sqlite3_bind_int(stmt, 11, uplink->preamble_len);
	sqlite3_bind_int64(stmt, 12, uplink->received_at);
	sqlite3_bind_blob(stmt, 13, uplink->device_id, sizeof(*uplink->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*uplink->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*uplink->id));
			status = 500;
			goto cleanup;
		}
		memcpy(uplink->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("uplink from device %02x%02x is conflicting\n", (*uplink->device_id)[0], (*uplink->device_id)[1]);
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

void uplink_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	uplink_query_t query = {.limit = 0, .offset = 0};
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

	uint8_t uplinks_len = 0;
	uint16_t status = uplink_select(database, bwt, &query, response, &uplinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu uplinks\n", uplinks_len);
	response->status = 200;
}

void uplink_find_one(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 12, &uuid_len);
	if (uuid_len != sizeof(*((uplink_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((uplink_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	uplink_t uplink = {.id = &id};
	uint16_t status = uplink_existing(database, bwt, &uplink);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = uplink_select_one(database, bwt, &uplink, response);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found uplink %02x%02x\n", (*uplink.id)[0], (*uplink.id)[1]);
	response->status = 200;
}

void uplink_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	uplink_query_t query = {.limit = 0, .offset = 0};
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
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint16_t uplinks_len = 0;
	status = uplink_select_by_device(database, bwt, &device, &query, response, &uplinks_len);
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
	info("found %hu uplinks\n", uplinks_len);
	response->status = 200;
}

void uplink_signal_find_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	uplink_signal_query_t query = {.from = 0, .to = 0, .bucket = 0};
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

	uint16_t signals_len = 0;
	status = uplink_signal_select_by_device(database, bwt, &device, &query, response, &signals_len);
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
	info("found %hu signals\n", signals_len);
	response->status = 200;
}

void uplink_signal_find_by_zone(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	uplink_signal_query_t query = {.from = 0, .to = 0, .bucket = 0};
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

	uint16_t signals_len = 0;
	status = uplink_signal_select_by_zone(database, bwt, &zone, &query, response, &signals_len);
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
	info("found %hu signals\n", signals_len);
	response->status = 200;
}

void uplink_create(sqlite3 *database, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

	uint8_t id[16];
	uplink_t uplink = {.id = &id};
	if (request->body.len == 0 || uplink_parse(&uplink, request) == -1 || uplink_validate(&uplink) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = uplink_insert(database, &uplink);
	if (status != 0) {
		response->status = status;
		return;
	}

	if ((status = decode(database, &uplink)) != 0) {
		response->status = status;
		return;
	}

	info("created uplink %02x%02x\n", (*uplink.id)[0], (*uplink.id)[1]);
	response->status = 201;
}
