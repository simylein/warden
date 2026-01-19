#include "radio.h"
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

const char *radio_file = "radio";

const radio_row_t radio_row = {
		.id = 0,
		.frequency = 16,
		.bandwidth = 20,
		.coding_rate = 24,
		.spreading_factor = 25,
		.preamble_length = 26,
		.tx_power = 27,
		.sync_word = 28,
		.checksum = 29,
		.captured_at = 30,
		.uplink_id = 38,
		.size = 54,
};

uint16_t radio_select_one_by_device(octet_t *db, device_t *device, response_t *response) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, radio_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select radio for device %02x%02x\n", (*device->id)[0], (*device->id)[1]);

	off_t offset = stmt.stat.st_size - radio_row.size;
	if (offset < 0) {
		debug("no radio for device %02x%02x found\n", (*device->id)[0], (*device->id)[1]);
		status = 204;
		goto cleanup;
	}

	if (octet_row_read(&stmt, file, offset, db->row, radio_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	uint32_t frequency = octet_uint32_read(db->row, radio_row.frequency);
	uint32_t bandwidth = octet_uint32_read(db->row, radio_row.bandwidth);
	uint8_t coding_rate = octet_uint8_read(db->row, radio_row.coding_rate);
	uint8_t spreading_factor = octet_uint8_read(db->row, radio_row.spreading_factor);
	uint8_t preamble_length = octet_uint8_read(db->row, radio_row.preamble_length);
	uint8_t tx_power = octet_uint8_read(db->row, radio_row.tx_power);
	uint8_t sync_word = octet_uint8_read(db->row, radio_row.sync_word);
	bool checksum = octet_uint8_read(db->row, radio_row.checksum);
	time_t captured_at = (time_t)octet_uint64_read(db->row, radio_row.captured_at);
	uint8_t (*uplink_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, radio_row.uplink_id);
	body_write(response, (uint32_t[]){hton32((uint32_t)frequency)}, sizeof(frequency));
	body_write(response, (uint32_t[]){hton32((uint32_t)bandwidth)}, sizeof(bandwidth));
	body_write(response, &coding_rate, sizeof(coding_rate));
	body_write(response, &spreading_factor, sizeof(spreading_factor));
	body_write(response, &preamble_length, sizeof(preamble_length));
	body_write(response, &tx_power, sizeof(tx_power));
	body_write(response, &sync_word, sizeof(sync_word));
	body_write(response, &checksum, sizeof(checksum));
	body_write(response, (uint64_t[]){hton64((uint64_t)captured_at)}, sizeof(captured_at));
	body_write(response, uplink_id, sizeof(*uplink_id));

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t radio_insert(octet_t *db, radio_t *radio) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*radio->id); index++) {
		(*radio->id)[index] = (uint8_t)(rand() % 0xff);
	}

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), radio->device_id, sizeof(*radio->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, radio_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert radio for device %02x%02x captured at %lu\n", (*radio->device_id)[0], (*radio->device_id)[1],
				radio->captured_at);

	octet_blob_write(db->row, radio_row.id, (uint8_t *)radio->id, sizeof(*radio->id));
	octet_uint32_write(db->row, radio_row.frequency, radio->frequency);
	octet_uint32_write(db->row, radio_row.bandwidth, radio->bandwidth);
	octet_uint8_write(db->row, radio_row.coding_rate, radio->coding_rate);
	octet_uint8_write(db->row, radio_row.spreading_factor, radio->spreading_factor);
	octet_uint8_write(db->row, radio_row.preamble_length, radio->preamble_length);
	octet_uint8_write(db->row, radio_row.tx_power, radio->tx_power);
	octet_uint8_write(db->row, radio_row.sync_word, radio->sync_word);
	octet_uint8_write(db->row, radio_row.checksum, radio->checksum);
	octet_uint64_write(db->row, radio_row.captured_at, (uint64_t)radio->captured_at);
	octet_blob_write(db->row, radio_row.uplink_id, (uint8_t *)radio->uplink_id, sizeof(*radio->uplink_id));

	off_t offset = stmt.stat.st_size;
	if (octet_row_write(&stmt, file, offset, db->row, radio_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void radio_find_one_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	status = radio_select_one_by_device(db, &device, response);
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
