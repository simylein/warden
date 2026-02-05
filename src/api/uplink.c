#include "uplink.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "cache.h"
#include "decode.h"
#include "device.h"
#include "user-device.h"
#include "user-zone.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *uplink_file = "uplink";

const uplink_row_t uplink_row = {
		.id = 0,
		.frame = 16,
		.kind = 18,
		.data_len = 19,
		.data = 20,
		.airtime = 52,
		.frequency = 54,
		.bandwidth = 58,
		.rssi = 62,
		.snr = 64,
		.sf = 65,
		.cr = 66,
		.tx_power = 67,
		.preamble_len = 68,
		.received_at = 69,
		.size = 77,
};

uint16_t uplink_select(octet_t *db, bwt_t *bwt, uplink_query_t *query, response_t *response, uint8_t *uplinks_len) {
	uint16_t status;

	uint8_t devices_len;
	user_t user = {.id = &bwt->id};
	status = user_device_select_by_user(db, &user, &devices_len);
	if (status != 0) {
		return status;
	}

	debug("select uplinks for user %02x%02x limit %hhu offset %u\n", bwt->id[0], bwt->id[1], query->limit, query->offset);

	char (*uuids)[32] = (char (*)[32])db->alpha;
	char (*files)[128] = (char (*)[128])db->bravo;
	off_t *offsets = (off_t *)db->charlie;
	time_t *received_ats = (time_t *)db->delta;
	octet_stmt_t *stmts = (octet_stmt_t *)db->echo;
	uint8_t stmts_len = 0;
	for (uint8_t index = 0; index < devices_len; index++) {
		uint8_t (*device_id)[16] =
				(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);

		if (base16_encode(uuids[index], sizeof(uuids[index]), device_id, sizeof(*device_id)) == -1) {
			error("failed to encode uuid to base 16\n");
			status = 500;
			goto cleanup;
		}

		if (sprintf(files[index], "%s/%.*s/%s.data", db->directory, (int)sizeof(uuids[index]), uuids[index], uplink_file) == -1) {
			error("failed to sprintf uuid to file\n");
			status = 500;
			goto cleanup;
		}

		if (octet_open(&stmts[index], files[index], O_RDONLY, F_RDLCK) == -1) {
			status = octet_error();
			goto cleanup;
		}

		offsets[index] = stmts[index].stat.st_size - uplink_row.size;
		stmts_len += 1;
	}

	for (uint8_t index = 0; index < stmts_len; index++) {
		if (offsets[index] < 0) {
			continue;
		}
		if (octet_row_read(&stmts[index], files[index], offsets[index], &db->table[index * uplink_row.size], uplink_row.size) ==
				-1) {
			status = octet_error();
			goto cleanup;
		}
		received_ats[index] = (time_t)octet_uint64_read(&db->table[index * uplink_row.size], uplink_row.received_at);
	}

	while (*uplinks_len < query->limit) {
		int8_t index = -1;
		time_t last_received_at = 0;
		for (int8_t ind = 0; ind < stmts_len; ind++) {
			if (offsets[ind] >= 0 && received_ats[ind] > last_received_at) {
				index = ind;
				last_received_at = received_ats[ind];
			}
		}

		if (index == -1) {
			status = 0;
			break;
		}

		if (query->offset == 0) {
			uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index * uplink_row.size], uplink_row.id);
			uint16_t frame = octet_uint16_read(&db->table[index * uplink_row.size], uplink_row.frame);
			uint8_t kind = octet_uint8_read(&db->table[index * uplink_row.size], uplink_row.kind);
			uint8_t data_len = octet_uint8_read(&db->table[index * uplink_row.size], uplink_row.data_len);
			uint8_t (*data)[32] = (uint8_t (*)[32])octet_blob_read(&db->table[index * uplink_row.size], uplink_row.data);
			uint16_t airtime = octet_uint16_read(&db->table[index * uplink_row.size], uplink_row.airtime);
			uint32_t frequency = octet_uint32_read(&db->table[index * uplink_row.size], uplink_row.frequency);
			uint32_t bandwidth = octet_uint32_read(&db->table[index * uplink_row.size], uplink_row.bandwidth);
			int16_t rssi = octet_int16_read(&db->table[index * uplink_row.size], uplink_row.rssi);
			int8_t snr = octet_int8_read(&db->table[index * uplink_row.size], uplink_row.snr);
			uint8_t sf = octet_uint8_read(&db->table[index * uplink_row.size], uplink_row.sf);
			uint8_t cr = octet_uint8_read(&db->table[index * uplink_row.size], uplink_row.cr);
			uint8_t tx_power = octet_uint8_read(&db->table[index * uplink_row.size], uplink_row.tx_power);
			uint8_t preamble_len = octet_uint8_read(&db->table[index * uplink_row.size], uplink_row.preamble_len);
			time_t received_at = (time_t)octet_uint64_read(&db->table[index * uplink_row.size], uplink_row.received_at);
			uint8_t (*device_id)[16] =
					(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);
			body_write(response, id, sizeof(*id));
			body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
			body_write(response, &kind, sizeof(kind));
			body_write(response, &data_len, sizeof(data_len));
			body_write(response, data, data_len);
			body_write(response, (uint16_t[]){hton16(airtime)}, sizeof(airtime));
			body_write(response, (uint32_t[]){hton32(frequency)}, sizeof(frequency));
			body_write(response, (uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
			body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
			body_write(response, &snr, sizeof(snr));
			body_write(response, &sf, sizeof(sf));
			body_write(response, &cr, sizeof(cr));
			body_write(response, &tx_power, sizeof(tx_power));
			body_write(response, &preamble_len, sizeof(preamble_len));
			body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
			body_write(response, device_id, sizeof(*device_id));
			*uplinks_len += 1;
		} else {
			query->offset -= 1;
		}
		offsets[index] -= uplink_row.size;

		if (octet_row_read(&stmts[index], files[index], offsets[index], &db->table[index * uplink_row.size], uplink_row.size) ==
				-1) {
			status = octet_error();
			goto cleanup;
		}
		received_ats[index] = (time_t)octet_uint64_read(&db->table[index * uplink_row.size], uplink_row.received_at);
	}

cleanup:
	for (uint8_t index = 0; index < stmts_len; index++) {
		octet_close(&stmts[index], files[index]);
	}
	return status;
}

uint16_t uplink_select_by_device(octet_t *db, device_t *device, uplink_query_t *query, response_t *response,
																 uint8_t *uplinks_len) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, uplink_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select uplinks for device %02x%02x limit %hhu offset %u\n", (*device->id)[0], (*device->id)[1], query->limit,
				query->offset);

	off_t offset = stmt.stat.st_size - uplink_row.size - query->offset * uplink_row.size;
	while (true) {
		if (offset < 0 || *uplinks_len >= query->limit) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, uplink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, uplink_row.id);
		uint16_t frame = octet_uint16_read(db->row, uplink_row.frame);
		uint8_t kind = octet_uint8_read(db->row, uplink_row.kind);
		uint8_t data_len = octet_uint8_read(db->row, uplink_row.data_len);
		uint8_t (*data)[32] = (uint8_t (*)[32])octet_blob_read(db->row, uplink_row.data);
		uint16_t airtime = octet_uint16_read(db->row, uplink_row.airtime);
		uint32_t frequency = octet_uint32_read(db->row, uplink_row.frequency);
		uint32_t bandwidth = octet_uint32_read(db->row, uplink_row.bandwidth);
		int16_t rssi = octet_int16_read(db->row, uplink_row.rssi);
		int8_t snr = octet_int8_read(db->row, uplink_row.snr);
		uint8_t sf = octet_uint8_read(db->row, uplink_row.sf);
		uint8_t cr = octet_uint8_read(db->row, uplink_row.cr);
		uint8_t tx_power = octet_uint8_read(db->row, uplink_row.tx_power);
		uint8_t preamble_len = octet_uint8_read(db->row, uplink_row.preamble_len);
		time_t received_at = (time_t)octet_uint64_read(db->row, uplink_row.received_at);
		body_write(response, id, sizeof(*id));
		body_write(response, (uint16_t[]){hton16(frame)}, sizeof(frame));
		body_write(response, &kind, sizeof(kind));
		body_write(response, &data_len, sizeof(data_len));
		body_write(response, data, data_len);
		body_write(response, (uint16_t[]){hton16(airtime)}, sizeof(airtime));
		body_write(response, (uint32_t[]){hton32(frequency)}, sizeof(frequency));
		body_write(response, (uint32_t[]){hton32(bandwidth)}, sizeof(bandwidth));
		body_write(response, (uint16_t[]){hton16((uint16_t)rssi)}, sizeof(rssi));
		body_write(response, &snr, sizeof(snr));
		body_write(response, &sf, sizeof(sf));
		body_write(response, &cr, sizeof(cr));
		body_write(response, &tx_power, sizeof(tx_power));
		body_write(response, &preamble_len, sizeof(preamble_len));
		body_write(response, (uint64_t[]){hton64((uint64_t)received_at)}, sizeof(received_at));
		*uplinks_len += 1;
		offset -= uplink_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t uplink_signal_select_by_device(octet_t *db, device_t *device, uplink_signal_query_t *query, response_t *response,
																				uint16_t *signals_len) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, uplink_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select signals for device %02x%02x from %lu to %lu bucket %hu\n", (*device->id)[0], (*device->id)[1], query->from,
				query->to, query->bucket);

	int32_t rssi_avg = 0;
	int16_t snr_avg = 0;
	uint16_t sf_avg = 0;
	time_t bucket_start = 0;
	time_t bucket_end = 0;
	uint8_t bucket_len = 0;
	off_t offset = stmt.stat.st_size - uplink_row.size;
	while (true) {
		if (offset < 0) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, uplink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		int16_t rssi = octet_int16_read(db->row, uplink_row.rssi);
		int8_t snr = octet_int8_read(db->row, uplink_row.snr);
		uint8_t sf = octet_uint8_read(db->row, uplink_row.sf);
		time_t received_at = (time_t)octet_uint64_read(db->row, uplink_row.received_at);
		if (response->body.len + sizeof(rssi) + sizeof(snr) + sizeof(sf) + sizeof(received_at) > response->body.cap) {
			error("signals amount %hu exceeds buffer length %u\n", *signals_len, response->body.cap);
			status = 500;
			goto cleanup;
		}
		if (received_at < query->from) {
			if (bucket_len != 0) {
				body_write(response, (uint16_t[]){hton16((uint16_t)(rssi_avg / bucket_len))}, sizeof(rssi));
				body_write(response, (int8_t[]){(int8_t)(snr_avg / bucket_len)}, sizeof(snr));
				body_write(response, (uint8_t[]){(uint8_t)(sf_avg / bucket_len)}, sizeof(sf));
				body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(received_at));
				*signals_len += 1;
			}
			status = 0;
			break;
		}
		if (received_at > query->from && received_at < query->to) {
			if (bucket_start == 0 || received_at + query->bucket <= bucket_start) {
				if (bucket_len != 0) {
					body_write(response, (uint16_t[]){hton16((uint16_t)(rssi_avg / bucket_len))}, sizeof(rssi));
					body_write(response, (int8_t[]){(int8_t)(snr_avg / bucket_len)}, sizeof(snr));
					body_write(response, (uint8_t[]){(uint8_t)(sf_avg / bucket_len)}, sizeof(sf));
					body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(received_at));
					*signals_len += 1;
				}
				rssi_avg = 0;
				snr_avg = 0;
				sf_avg = 0;
				bucket_len = 0;
				bucket_start = received_at;
			}
			rssi_avg += rssi;
			snr_avg += snr;
			sf_avg += sf;
			bucket_len += 1;
			bucket_end = received_at;
		}
		offset -= uplink_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t uplink_signal_select_by_zone(octet_t *db, zone_t *zone, uplink_signal_query_t *query, response_t *response,
																			uint16_t *signals_len) {
	uint16_t status;

	uint8_t devices_len;
	status = device_select_by_zone(db, zone, &devices_len);
	if (status != 0) {
		return status;
	}

	debug("select signals for zone %02x%02x from %lu to %lu bucket %hu\n", (*zone->id)[0], (*zone->id)[1], query->from, query->to,
				query->bucket);

	char uuid[32];
	char file[128];
	octet_stmt_t stmt;
	for (uint8_t index = 0; index < devices_len; index++) {
		uint8_t (*device_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->chunk[index * device_row.size], device_row.id);

		if (base16_encode(uuid, sizeof(uuid), device_id, sizeof(*device_id)) == -1) {
			error("failed to encode uuid to base 16\n");
			return 500;
		}

		if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, uplink_file) == -1) {
			error("failed to sprintf uuid to file\n");
			return 500;
		}

		uint16_t signals = 0;
		if (response->body.len + sizeof(*device_id) + sizeof(signals) > response->body.cap) {
			error("signals amount %hu exceeds buffer length %u\n", *signals_len, response->body.cap);
			return 500;
		}

		body_write(response, device_id, sizeof(*device_id));
		uint32_t signals_ind = response->body.len;
		response->body.len += sizeof(signals);

		if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
			status = octet_error();
			goto cleanup;
		}

		int32_t rssi_avg = 0;
		int16_t snr_avg = 0;
		uint16_t sf_avg = 0;
		time_t bucket_start = 0;
		time_t bucket_end = 0;
		uint8_t bucket_len = 0;
		off_t offset = stmt.stat.st_size - uplink_row.size;
		while (true) {
			if (offset < 0) {
				status = 0;
				break;
			}
			if (octet_row_read(&stmt, file, offset, db->row, uplink_row.size) == -1) {
				status = octet_error();
				goto cleanup;
			}
			int16_t rssi = octet_int16_read(db->row, uplink_row.rssi);
			int8_t snr = octet_int8_read(db->row, uplink_row.snr);
			uint8_t sf = octet_uint8_read(db->row, uplink_row.sf);
			time_t received_at = (time_t)octet_uint64_read(db->row, uplink_row.received_at);
			if (response->body.len + sizeof(rssi) + sizeof(snr) + sizeof(sf) + sizeof(received_at) > response->body.cap) {
				error("signals amount %hu exceeds buffer length %u\n", *signals_len, response->body.cap);
				status = 500;
				goto cleanup;
			}
			if (received_at < query->from) {
				if (bucket_len != 0) {
					body_write(response, (uint16_t[]){hton16((uint16_t)(rssi_avg / bucket_len))}, sizeof(rssi));
					body_write(response, (int8_t[]){(int8_t)(snr_avg / bucket_len)}, sizeof(snr));
					body_write(response, (uint8_t[]){(uint8_t)(sf_avg / bucket_len)}, sizeof(sf));
					body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(received_at));
					signals += 1;
					*signals_len += 1;
				}
				status = 0;
				break;
			}
			if (received_at > query->from && received_at < query->to) {
				if (bucket_start == 0 || received_at + query->bucket <= bucket_start) {
					if (bucket_len != 0) {
						body_write(response, (uint16_t[]){hton16((uint16_t)(rssi_avg / bucket_len))}, sizeof(rssi));
						body_write(response, (int8_t[]){(int8_t)(snr_avg / bucket_len)}, sizeof(snr));
						body_write(response, (uint8_t[]){(uint8_t)(sf_avg / bucket_len)}, sizeof(sf));
						body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(received_at));
						signals += 1;
						*signals_len += 1;
					}
					rssi_avg = 0;
					snr_avg = 0;
					sf_avg = 0;
					bucket_len = 0;
					bucket_start = received_at;
				}
				rssi_avg += rssi;
				snr_avg += snr;
				sf_avg += sf;
				bucket_len += 1;
				bucket_end = received_at;
			}
			offset -= uplink_row.size;
		}

	cleanup:
		memcpy(response->body.ptr + signals_ind, (uint16_t[]){hton16(signals)}, sizeof(signals));
		octet_close(&stmt, file);
		if (status != 0) {
			break;
		}
	}

	return status;
}

int uplink_parse(uplink_t *uplink, request_t *request) {
	request->body.pos = 0;

	if (request->body.len < request->body.pos + sizeof(uplink->frame)) {
		debug("missing frame on uplink\n");
		return -1;
	}
	memcpy(&uplink->frame, body_read(request, sizeof(uplink->frame)), sizeof(uplink->frame));
	uplink->frame = ntoh16(uplink->frame);

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
	if (uplink->data_len > 32) {
		debug("invalid data len %hhu on uplink\n", uplink->data_len);
		return -1;
	}

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

uint16_t uplink_insert(octet_t *db, uplink_t *uplink) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*uplink->id); index++) {
		(*uplink->id)[index] = (uint8_t)(rand() & 0xff);
	}

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), uplink->device_id, sizeof(*uplink->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, uplink_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert uplink for device %02x%02x received at %lu\n", (*uplink->device_id)[0], (*uplink->device_id)[1],
				uplink->received_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - uplink_row.size, db->row, uplink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t received_at = (time_t)octet_uint64_read(db->row, uplink_row.received_at);
		if (received_at <= uplink->received_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, uplink_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= uplink_row.size;
	}

	octet_blob_write(db->row, uplink_row.id, (uint8_t *)uplink->id, sizeof(*uplink->id));
	octet_uint16_write(db->row, uplink_row.frame, uplink->frame);
	octet_uint8_write(db->row, uplink_row.kind, uplink->kind);
	octet_uint8_write(db->row, uplink_row.data_len, uplink->data_len);
	octet_blob_write(db->row, uplink_row.data, uplink->data, uplink->data_len);
	octet_uint16_write(db->row, uplink_row.airtime, uplink->airtime);
	octet_uint32_write(db->row, uplink_row.frequency, uplink->frequency);
	octet_uint32_write(db->row, uplink_row.bandwidth, uplink->bandwidth);
	octet_int16_write(db->row, uplink_row.rssi, uplink->rssi);
	octet_int8_write(db->row, uplink_row.snr, uplink->snr);
	octet_uint8_write(db->row, uplink_row.sf, uplink->sf);
	octet_uint8_write(db->row, uplink_row.cr, uplink->cr);
	octet_uint8_write(db->row, uplink_row.tx_power, uplink->tx_power);
	octet_uint8_write(db->row, uplink_row.preamble_len, uplink->preamble_len);
	octet_uint64_write(db->row, uplink_row.received_at, (uint64_t)uplink->received_at);

	if (octet_row_write(&stmt, file, offset, db->row, uplink_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	uint8_t zone_id[16];
	device_t device = {.id = uplink->device_id, .zone_id = &zone_id};
	status = device_update_latest(db, &device, NULL, NULL, NULL, uplink, NULL);
	if (status != 0) {
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void uplink_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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
	uint16_t status = uplink_select(db, bwt, &query, response, &uplinks_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu uplinks\n", uplinks_len);
	response->status = 200;
}

void uplink_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	uint8_t uplinks_len = 0;
	status = uplink_select_by_device(db, &device, &query, response, &uplinks_len);
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

void uplink_signal_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	uint16_t signals_len = 0;
	status = uplink_signal_select_by_device(db, &device, &query, response, &signals_len);
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

void uplink_signal_find_by_zone(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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
	uint16_t status = zone_existing(db, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	user_zone_t user_zone = {.user_id = &bwt->id, .zone_id = zone.id};
	status = user_zone_existing(db, &user_zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint16_t signals_len = 0;
	status = uplink_signal_select_by_zone(db, &zone, &query, response, &signals_len);
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

void uplink_create(octet_t *db, request_t *request, response_t *response) {
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

	uint16_t status = uplink_insert(db, &uplink);
	if (status != 0) {
		response->status = status;
		return;
	}

	if ((status = decode(db, &uplink)) != 0) {
		response->status = status;
		return;
	}

	info("created uplink %02x%02x\n", (*uplink.id)[0], (*uplink.id)[1]);
	response->status = 201;
}
