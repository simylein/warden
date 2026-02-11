#include "radio.h"
#include "../app/auth.h"
#include "../app/http.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "device.h"
#include "host.h"
#include "user-device.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *radio_file = "radio";

const radio_row_t radio_row = {
		.frequency = 0,
		.bandwidth = 4,
		.coding_rate = 8,
		.spreading_factor = 9,
		.preamble_length = 10,
		.tx_power = 11,
		.sync_word = 12,
		.checksum = 13,
		.captured_at = 14,
		.size = 22,
};

uint16_t radio_select_one_by_device(octet_t *db, device_t *device, response_t *response) {
	uint16_t status;

	char uuid[16];
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

cleanup:
	octet_close(&stmt, file);
	return status;
}

int radio_parse(radio_t *radio, request_t *request) {
	request->body.pos = 0;

	if (request->body.len < request->body.pos + sizeof(radio->frequency)) {
		debug("missing frequency on radio\n");
		return -1;
	}
	memcpy(&radio->frequency, body_read(request, sizeof(radio->frequency)), sizeof(radio->frequency));
	radio->frequency = ntoh32(radio->frequency);

	if (request->body.len < request->body.pos + sizeof(radio->bandwidth)) {
		debug("missing bandwidth on radio\n");
		return -1;
	}
	memcpy(&radio->bandwidth, body_read(request, sizeof(radio->bandwidth)), sizeof(radio->bandwidth));
	radio->bandwidth = ntoh32(radio->bandwidth);

	if (request->body.len < request->body.pos + sizeof(radio->coding_rate)) {
		debug("missing coding rate enable on radio\n");
		return -1;
	}
	radio->coding_rate = (uint8_t)*body_read(request, sizeof(radio->coding_rate));

	if (request->body.len < request->body.pos + sizeof(radio->spreading_factor)) {
		debug("missing spreading factor enable on radio\n");
		return -1;
	}
	radio->spreading_factor = (uint8_t)*body_read(request, sizeof(radio->spreading_factor));

	if (request->body.len < request->body.pos + sizeof(radio->preamble_length)) {
		debug("missing preamble length enable on radio\n");
		return -1;
	}
	radio->preamble_length = (uint8_t)*body_read(request, sizeof(radio->preamble_length));

	if (request->body.len < request->body.pos + sizeof(radio->tx_power)) {
		debug("missing tx power enable on radio\n");
		return -1;
	}
	radio->tx_power = (uint8_t)*body_read(request, sizeof(radio->tx_power));

	if (request->body.len < request->body.pos + sizeof(radio->sync_word)) {
		debug("missing sync word enable on radio\n");
		return -1;
	}
	radio->sync_word = (uint8_t)*body_read(request, sizeof(radio->sync_word));

	if (request->body.len < request->body.pos + sizeof(radio->checksum)) {
		debug("missing checksum enable on radio\n");
		return -1;
	}
	radio->checksum = *body_read(request, sizeof(radio->checksum));

	if (request->body.len != request->body.pos) {
		debug("body len %u does not match body pos %u\n", request->body.len, request->body.pos);
		return -1;
	}

	return 0;
}

int radio_validate(radio_t *radio) {
	if (radio->frequency < 430000000 || radio->frequency > 440000000) {
		debug("invalid frequency %u on radio\n", radio->frequency);
		return -1;
	}

	if (radio->bandwidth < 7800 || radio->bandwidth > 500000) {
		debug("invalid bandwidth %u on radio\n", radio->bandwidth);
		return -1;
	}

	if (radio->coding_rate < 5 || radio->coding_rate > 8) {
		debug("invalid coding rate %hhu on radio\n", radio->coding_rate);
		return -1;
	}

	if (radio->spreading_factor < 6 || radio->spreading_factor > 12) {
		debug("invalid spreading factor %hhu on radio\n", radio->spreading_factor);
		return -1;
	}

	if (radio->preamble_length < 6 || radio->preamble_length > 21) {
		debug("invalid preamble length %hhu on radio\n", radio->preamble_length);
		return -1;
	}

	if (radio->tx_power < 2 || radio->tx_power > 17) {
		debug("invalid tx power %hhu on radio\n", radio->tx_power);
		return -1;
	}

	return 0;
}

uint16_t radio_insert(octet_t *db, radio_t *radio) {
	uint16_t status;

	char uuid[16];
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

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - radio_row.size, db->row, radio_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t captured_at = (time_t)octet_uint64_read(db->row, radio_row.captured_at);
		if (captured_at <= radio->captured_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, radio_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= radio_row.size;
	}

	octet_uint32_write(db->row, radio_row.frequency, radio->frequency);
	octet_uint32_write(db->row, radio_row.bandwidth, radio->bandwidth);
	octet_uint8_write(db->row, radio_row.coding_rate, radio->coding_rate);
	octet_uint8_write(db->row, radio_row.spreading_factor, radio->spreading_factor);
	octet_uint8_write(db->row, radio_row.preamble_length, radio->preamble_length);
	octet_uint8_write(db->row, radio_row.tx_power, radio->tx_power);
	octet_uint8_write(db->row, radio_row.sync_word, radio->sync_word);
	octet_uint8_write(db->row, radio_row.checksum, radio->checksum);
	octet_uint64_write(db->row, radio_row.captured_at, (uint64_t)radio->captured_at);

	if (octet_row_write(&stmt, file, offset, db->row, radio_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t radio_update(radio_t *radio, host_t *host, cookie_t *cookie) {
	request_t request;
	response_t response;

	char method[] = "POST";
	request.method.ptr = method;
	request.method.len = sizeof(method) - 1;

	char pathname[] = "/api/schedule";
	request.pathname.ptr = pathname;
	request.pathname.len = sizeof(pathname) - 1;

	char protocol[] = "HTTP/1.1";
	request.protocol.ptr = protocol;
	request.protocol.len = sizeof(protocol) - 1;

	char request_header[256];
	request.header.ptr = request_header;
	request.header.len = (uint16_t)sprintf(request.header.ptr, "cookie:auth=%.*s\r\n", cookie->len, cookie->ptr);
	request.header.cap = sizeof(request_header);

	char request_body[512];
	request.body.ptr = request_body;
	request.body.len = 0;
	request.body.cap = sizeof(request_body);

	char response_header[256];
	response.header.ptr = response_header;
	response.header.len = 0;
	response.header.cap = sizeof(response_header);

	char response_body[512];
	response.body.ptr = response_body;
	response.body.len = 0;
	response.body.cap = sizeof(response_body);

	request.body.ptr[request.body.len] = 0x06;
	request.body.len += sizeof(uint8_t);
	request.body.ptr[request.body.len] = sizeof(radio->frequency) + sizeof(uint16_t) + sizeof(uint8_t) +
																			 sizeof(radio->coding_rate) + sizeof(radio->spreading_factor) +
																			 sizeof(radio->preamble_length) + sizeof(radio->tx_power) + sizeof(radio->sync_word) +
																			 sizeof(radio->checksum);
	request.body.len += sizeof(uint8_t);
	memcpy(&request.body.ptr[request.body.len], (uint32_t[]){hton32(radio->frequency)}, sizeof(radio->frequency));
	request.body.len += sizeof(radio->frequency);
	request.body.ptr[request.body.len] = (char)(radio->bandwidth >> 16 & 0xff);
	request.body.len += sizeof(uint8_t);
	request.body.ptr[request.body.len] = (char)(radio->bandwidth >> 8 & 0xff);
	request.body.len += sizeof(uint8_t);
	request.body.ptr[request.body.len] = (char)(radio->bandwidth & 0xff);
	request.body.len += sizeof(uint8_t);
	request.body.ptr[request.body.len] = (char)radio->coding_rate;
	request.body.len += sizeof(radio->coding_rate);
	request.body.ptr[request.body.len] = (char)radio->spreading_factor;
	request.body.len += sizeof(radio->spreading_factor);
	request.body.ptr[request.body.len] = (char)radio->preamble_length;
	request.body.len += sizeof(radio->preamble_length);
	request.body.ptr[request.body.len] = (char)radio->tx_power;
	request.body.len += sizeof(radio->tx_power);
	request.body.ptr[request.body.len] = (char)radio->sync_word;
	request.body.len += sizeof(radio->sync_word);
	request.body.ptr[request.body.len] = radio->checksum;
	request.body.len += sizeof(radio->checksum);
	memcpy(&request.body.ptr[request.body.len], radio->device_id, sizeof(*radio->device_id));
	request.body.len += sizeof(*radio->device_id);

	char buffer[33];
	sprintf(buffer, "%.*s", host->address_len, host->address);
	if (fetch(buffer, host->port, &request, &response) == -1) {
		return 0;
	}

	return response.status;
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

	uint8_t id[8];
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

void radio_modify(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	uint8_t id[8];
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

	radio_t radio = {.device_id = device.id};
	if (request->body.len == 0 || radio_parse(&radio, request) == -1 || radio_validate(&radio) == -1) {
		response->status = 400;
		return;
	}

	char address[32];
	char username[16];
	char password[64];
	host_t host = {.address = (char *)&address, .username = (char *)&username, .password = (char *)&password};
	status = host_select_one(db, &host);
	if (status != 0) {
		response->status = status;
		return;
	}

	char buffer[128];
	cookie_t cookie = {.ptr = (char *)&buffer, .len = 0, .cap = sizeof(buffer), .age = 0};
	if (auth(&host, &cookie) == -1) {
		response->status = 503;
		return;
	}

	status = radio_update(&radio, &host, &cookie);
	if (status != 201) {
		error("host rejected schedule with status %hu\n", status);
		response->status = 503;
		return;
	}

	info("updated radio for device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 202;
}
