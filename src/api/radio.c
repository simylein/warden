#include "radio.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "database.h"
#include "device.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *radio_table = "radio";
const char *radio_schema = "create table radio ("
													 "id blob primary key, "
													 "frequency integer not null, "
													 "bandwidth integer not null, "
													 "coding_rate integer not null, "
													 "spreading_factor integer not null, "
													 "preamble_length integer not null, "
													 "tx_power integer not null, "
													 "sync_word integer not null, "
													 "checksum boolean not null, "
													 "captured_at timestamp not null, "
													 "uplink_id blob not null unique, "
													 "device_id blob not null, "
													 "foreign key (uplink_id) references uplink(id) on delete cascade, "
													 "foreign key (device_id) references device(id) on delete cascade"
													 ")";

uint16_t radio_select_one_by_device(sqlite3 *database, bwt_t *bwt, device_t *device, response_t *response) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"radio.frequency, radio.bandwidth, radio.coding_rate, radio.spreading_factor, "
										"radio.preamble_length, radio.tx_power, radio.sync_word, radio.checksum, radio.captured_at "
										"from radio "
										"join user_device on user_device.device_id = radio.device_id and user_device.user_id = ? "
										"where radio.device_id = ? "
										"order by captured_at desc "
										"limit 1";
	debug("select radio for device %02x%02x for user %02x%02x\n", (*device->id)[0], (*device->id)[1], bwt->id[0], bwt->id[1]);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, bwt->id, sizeof(bwt->id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 2, device->id, sizeof(*device->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint32_t frequency = (uint32_t)sqlite3_column_int(stmt, 0);
		const uint32_t bandwidth = (uint32_t)sqlite3_column_int(stmt, 1);
		const uint8_t coding_rate = (uint8_t)sqlite3_column_int(stmt, 2);
		const uint8_t spreading_factor = (uint8_t)sqlite3_column_int(stmt, 3);
		const uint8_t preamble_length = (uint8_t)sqlite3_column_int(stmt, 4);
		const uint8_t tx_power = (uint8_t)sqlite3_column_int(stmt, 5);
		const uint8_t sync_word = (uint8_t)sqlite3_column_int(stmt, 6);
		const bool checksum = (bool)sqlite3_column_int(stmt, 7);
		const time_t captured_at = (time_t)sqlite3_column_int64(stmt, 8);
		body_write(response, (uint32_t[]){hton32((uint32_t)frequency)}, sizeof(frequency));
		body_write(response, (uint32_t[]){hton32((uint32_t)bandwidth)}, sizeof(bandwidth));
		body_write(response, &coding_rate, sizeof(coding_rate));
		body_write(response, &spreading_factor, sizeof(spreading_factor));
		body_write(response, &preamble_length, sizeof(preamble_length));
		body_write(response, &tx_power, sizeof(tx_power));
		body_write(response, &sync_word, sizeof(sync_word));
		body_write(response, &checksum, sizeof(checksum));
		body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
		status = 0;
	} else if (result == SQLITE_DONE) {
		debug("no radio for device %02x%02x found\n", (*device->id)[0], (*device->id)[1]);
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

uint16_t radio_insert(sqlite3 *database, radio_t *radio) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "insert into radio (id, frequency, bandwidth, coding_rate, spreading_factor, "
										"preamble_length, tx_power, sync_word, checksum, captured_at, uplink_id, device_id) "
										"values (randomblob(16), ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) returning id";
	debug("insert radio for device %02x%02x captured at %lu\n", (*radio->device_id)[0], (*radio->device_id)[1],
				radio->captured_at);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_int(stmt, 1, (int)radio->frequency);
	sqlite3_bind_int(stmt, 2, (int)radio->bandwidth);
	sqlite3_bind_int(stmt, 3, radio->coding_rate);
	sqlite3_bind_int(stmt, 4, radio->spreading_factor);
	sqlite3_bind_int(stmt, 5, radio->preamble_length);
	sqlite3_bind_int(stmt, 6, radio->tx_power);
	sqlite3_bind_int(stmt, 7, radio->sync_word);
	sqlite3_bind_int(stmt, 8, radio->checksum);
	sqlite3_bind_int64(stmt, 9, radio->captured_at);
	sqlite3_bind_blob(stmt, 10, radio->uplink_id, sizeof(*radio->uplink_id), SQLITE_STATIC);
	sqlite3_bind_blob(stmt, 11, radio->device_id, sizeof(*radio->device_id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result == SQLITE_ROW) {
		const uint8_t *id = sqlite3_column_blob(stmt, 0);
		const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
		if (id_len != sizeof(*radio->id)) {
			error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*radio->id));
			status = 500;
			goto cleanup;
		}
		memcpy(radio->id, id, id_len);
		status = 0;
	} else if (result == SQLITE_CONSTRAINT) {
		warn("radio is conflicting because %s\n", sqlite3_errmsg(database));
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

void radio_find_one_by_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
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

	status = radio_select_one_by_device(database, bwt, &device, response);
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
	info("found radio for device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}
