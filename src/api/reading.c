#include "reading.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "../lib/strn.h"
#include "cache.h"
#include "device.h"
#include "user-device.h"
#include "user-zone.h"
#include "zone.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *reading_file = "reading";

const reading_row_t reading_row = {
		.temperature = 0,
		.humidity = 2,
		.captured_at = 4,
		.size = 12,
};

uint16_t reading_select(octet_t *db, bwt_t *bwt, reading_query_t *query, response_t *response, uint16_t *readings_len) {
	uint16_t status;

	uint8_t user_devices_len = 0;
	user_t user = {.id = &bwt->id};
	status = user_device_select_by_user(db, &user, &user_devices_len);
	if (status != 0) {
		return status;
	}

	debug("select readings for user %02x%02x from %lu to %lu bucket %hu\n", bwt->id[0], bwt->id[1], query->from, query->to,
				query->bucket);

	char uuid[32];
	char file[128];
	octet_stmt_t stmt;
	for (uint8_t index = 0; index < user_devices_len; index++) {
		uint8_t (*device_id)[16] =
				(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);

		if (base16_encode(uuid, sizeof(uuid), device_id, sizeof(*device_id)) == -1) {
			error("failed to encode uuid to base 16\n");
			return 500;
		}

		if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, reading_file) == -1) {
			error("failed to sprintf uuid to file\n");
			return 500;
		}

		uint16_t readings = 0;
		if (response->body.len + sizeof(*device_id) + sizeof(readings) > response->body.cap) {
			error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
			return 500;
		}

		body_write(response, device_id, sizeof(*device_id));
		device_t device = {.id = device_id};
		cache_device_t cache_device;
		int cache_hit = cache_device_read(&cache_device, &device);
		body_write(response, (uint8_t[]){cache_hit != -1}, sizeof(uint8_t));
		if (cache_hit != -1) {
			body_write(response, cache_device.name, cache_device.name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint8_t[]){cache_device.zone_name_len != 0}, sizeof(cache_device.zone_name_len));
			if (cache_device.zone_name_len != 0) {
				body_write(response, cache_device.zone_name, cache_device.zone_name_len);
				body_write(response, (char[]){0x00}, sizeof(char));
			}
		}

		uint32_t readings_ind = response->body.len;
		response->body.len += sizeof(readings);

		if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
			status = octet_error();
			goto cleanup;
		}

		int32_t temperature_avg = 0;
		uint32_t humidity_avg = 0;
		time_t bucket_start = 0;
		time_t bucket_end = 0;
		uint8_t bucket_len = 0;
		off_t offset = stmt.stat.st_size - reading_row.size;
		while (true) {
			if (offset < 0) {
				status = 0;
				break;
			}
			if (octet_row_read(&stmt, file, offset, db->row, reading_row.size) == -1) {
				status = octet_error();
				goto cleanup;
			}
			int16_t temperature = octet_int16_read(db->row, reading_row.temperature);
			uint16_t humidity = octet_uint16_read(db->row, reading_row.humidity);
			time_t captured_at = (time_t)octet_uint64_read(db->row, reading_row.captured_at);
			if (response->body.len + sizeof(temperature) + sizeof(humidity) + sizeof(captured_at) > response->body.cap) {
				error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
				status = 500;
				goto cleanup;
			}
			if (captured_at < query->from) {
				if (bucket_len != 0) {
					body_write(response, (uint16_t[]){hton16((uint16_t)(temperature_avg / bucket_len))}, sizeof(temperature));
					body_write(response, (uint16_t[]){hton16((uint16_t)(humidity_avg / bucket_len))}, sizeof(humidity));
					body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(captured_at));
					readings += 1;
					*readings_len += 1;
				}
				status = 0;
				break;
			}
			if (captured_at >= query->from && captured_at <= query->to) {
				if (bucket_start == 0 || captured_at + query->bucket <= bucket_start) {
					if (bucket_len != 0) {
						body_write(response, (uint16_t[]){hton16((uint16_t)(temperature_avg / bucket_len))}, sizeof(temperature));
						body_write(response, (uint16_t[]){hton16((uint16_t)(humidity_avg / bucket_len))}, sizeof(humidity));
						body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(captured_at));
						readings += 1;
						*readings_len += 1;
					}
					temperature_avg = 0;
					humidity_avg = 0;
					bucket_len = 0;
					bucket_start = captured_at;
				}
				temperature_avg += temperature;
				humidity_avg += humidity;
				bucket_len += 1;
				bucket_end = captured_at;
			}
			offset -= reading_row.size;
		}

	cleanup:
		memcpy(response->body.ptr + readings_ind, (uint16_t[]){hton16(readings)}, sizeof(readings));
		octet_close(&stmt, file);
		if (status != 0) {
			break;
		}
	}

	return status;
}

uint16_t reading_select_by_device(octet_t *db, device_t *device, reading_query_t *query, response_t *response,
																	uint16_t *readings_len) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, reading_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select readings for device %02x%02x from %lu to %lu bucket %hu\n", (*device->id)[0], (*device->id)[1], query->from,
				query->to, query->bucket);

	int32_t temperature_avg = 0;
	uint32_t humidity_avg = 0;
	time_t bucket_start = 0;
	time_t bucket_end = 0;
	uint8_t bucket_len = 0;
	off_t offset = stmt.stat.st_size - reading_row.size;
	while (true) {
		if (offset < 0) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, reading_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		int16_t temperature = octet_int16_read(db->row, reading_row.temperature);
		uint16_t humidity = octet_uint16_read(db->row, reading_row.humidity);
		time_t captured_at = (time_t)octet_uint64_read(db->row, reading_row.captured_at);
		if (response->body.len + sizeof(temperature) + sizeof(humidity) + sizeof(captured_at) > response->body.cap) {
			error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
			status = 500;
			goto cleanup;
		}
		if (captured_at < query->from) {
			if (bucket_len != 0) {
				body_write(response, (uint16_t[]){hton16((uint16_t)(temperature_avg / bucket_len))}, sizeof(temperature));
				body_write(response, (uint16_t[]){hton16((uint16_t)(humidity_avg / bucket_len))}, sizeof(humidity));
				body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(captured_at));
				*readings_len += 1;
			}
			status = 0;
			break;
		}
		if (captured_at >= query->from && captured_at <= query->to) {
			if (bucket_start == 0 || captured_at + query->bucket <= bucket_start) {
				if (bucket_len != 0) {
					body_write(response, (uint16_t[]){hton16((uint16_t)(temperature_avg / bucket_len))}, sizeof(temperature));
					body_write(response, (uint16_t[]){hton16((uint16_t)(humidity_avg / bucket_len))}, sizeof(humidity));
					body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(captured_at));
					*readings_len += 1;
				}
				temperature_avg = 0;
				humidity_avg = 0;
				bucket_len = 0;
				bucket_start = captured_at;
			}
			temperature_avg += temperature;
			humidity_avg += humidity;
			bucket_len += 1;
			bucket_end = captured_at;
		}
		offset -= reading_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t reading_select_by_zone(octet_t *db, zone_t *zone, reading_query_t *query, response_t *response,
																uint16_t *readings_len) {
	uint16_t status;

	uint8_t devices_len;
	status = device_select_by_zone(db, zone, &devices_len);
	if (status != 0) {
		return status;
	}

	debug("select readings for zone %02x%02x from %lu to %lu bucket %hu\n", (*zone->id)[0], (*zone->id)[1], query->from,
				query->to, query->bucket);

	char uuid[32];
	char file[128];
	octet_stmt_t stmt;
	for (uint8_t index = 0; index < devices_len; index++) {
		uint8_t (*device_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->chunk[index * device_row.size], device_row.id);

		if (base16_encode(uuid, sizeof(uuid), device_id, sizeof(*device_id)) == -1) {
			error("failed to encode uuid to base 16\n");
			return 500;
		}

		if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, reading_file) == -1) {
			error("failed to sprintf uuid to file\n");
			return 500;
		}

		uint16_t readings = 0;
		if (response->body.len + sizeof(*device_id) + sizeof(readings) > response->body.cap) {
			error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
			return 500;
		}

		body_write(response, device_id, sizeof(*device_id));
		uint32_t readings_ind = response->body.len;
		response->body.len += sizeof(readings);

		if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
			status = octet_error();
			goto cleanup;
		}

		int32_t temperature_avg = 0;
		uint32_t humidity_avg = 0;
		time_t bucket_start = 0;
		time_t bucket_end = 0;
		uint8_t bucket_len = 0;
		off_t offset = stmt.stat.st_size - reading_row.size;
		while (true) {
			if (offset < 0) {
				status = 0;
				break;
			}
			if (octet_row_read(&stmt, file, offset, db->row, reading_row.size) == -1) {
				status = octet_error();
				goto cleanup;
			}
			int16_t temperature = octet_int16_read(db->row, reading_row.temperature);
			uint16_t humidity = octet_uint16_read(db->row, reading_row.humidity);
			time_t captured_at = (time_t)octet_uint64_read(db->row, reading_row.captured_at);
			if (response->body.len + sizeof(temperature) + sizeof(humidity) + sizeof(captured_at) > response->body.cap) {
				error("readings amount %hu exceeds buffer length %u\n", *readings_len, response->body.cap);
				status = 500;
				goto cleanup;
			}
			if (captured_at < query->from) {
				if (bucket_len != 0) {
					body_write(response, (uint16_t[]){hton16((uint16_t)(temperature_avg / bucket_len))}, sizeof(temperature));
					body_write(response, (uint16_t[]){hton16((uint16_t)(humidity_avg / bucket_len))}, sizeof(humidity));
					body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(captured_at));
					readings += 1;
					*readings_len += 1;
				}
				status = 0;
				break;
			}
			if (captured_at >= query->from && captured_at <= query->to) {
				if (bucket_start == 0 || captured_at + query->bucket <= bucket_start) {
					if (bucket_len != 0) {
						body_write(response, (uint16_t[]){hton16((uint16_t)(temperature_avg / bucket_len))}, sizeof(temperature));
						body_write(response, (uint16_t[]){hton16((uint16_t)(humidity_avg / bucket_len))}, sizeof(humidity));
						body_write(response, (uint64_t[]){hton64((uint64_t)((bucket_start + bucket_end) / 2))}, sizeof(captured_at));
						readings += 1;
						*readings_len += 1;
					}
					temperature_avg = 0;
					humidity_avg = 0;
					bucket_len = 0;
					bucket_start = captured_at;
				}
				temperature_avg += temperature;
				humidity_avg += humidity;
				bucket_len += 1;
				bucket_end = captured_at;
			}
			offset -= reading_row.size;
		}

	cleanup:
		memcpy(response->body.ptr + readings_ind, (uint16_t[]){hton16(readings)}, sizeof(readings));
		octet_close(&stmt, file);
		if (status != 0) {
			break;
		}
	}

	return status;
}

uint16_t reading_insert(octet_t *db, reading_t *reading) {
	uint16_t status;

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), reading->device_id, sizeof(*reading->device_id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, reading_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert reading for device %02x%02x captured at %lu\n", (*reading->device_id)[0], (*reading->device_id)[1],
				reading->captured_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - reading_row.size, db->row, reading_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		time_t captured_at = (time_t)octet_uint64_read(db->row, reading_row.captured_at);
		if (captured_at <= reading->captured_at) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, reading_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= reading_row.size;
	}

	octet_int16_write(db->row, reading_row.temperature, (int16_t)(reading->temperature * 100));
	octet_uint16_write(db->row, reading_row.humidity, (uint16_t)(reading->humidity * 100));
	octet_uint64_write(db->row, reading_row.captured_at, (uint64_t)reading->captured_at);

	if (octet_row_write(&stmt, file, offset, db->row, reading_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	uint8_t zone_id[16];
	device_t device = {.id = reading->device_id, .zone_id = &zone_id};
	status = device_update_latest(db, &device, reading, NULL, NULL, NULL, NULL);
	if (status != 0) {
		goto cleanup;
	}

	if (device.zone_id != NULL) {
		zone_t zone = {.id = device.zone_id};
		status = zone_update_latest(db, &zone);
		if (status != 0) {
			goto cleanup;
		}
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

void reading_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	const char *from;
	size_t from_len;
	if (strnfind(request->search.ptr, request->search.len, "from=", "&", &from, &from_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *to;
	size_t to_len;
	if (strnfind(request->search.ptr, request->search.len, "to=", "&", &to, &to_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *bucket;
	size_t bucket_len;
	if (strnfind(request->search.ptr, request->search.len, "bucket=", "", &bucket, &bucket_len, 8) == -1) {
		response->status = 400;
		return;
	}

	reading_query_t query = {.from = 0, .to = 0, .bucket = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1 || strnto64(to, to_len, (uint64_t *)&query.to) == -1 ||
			strnto16(bucket, bucket_len, &query.bucket) == -1) {
		warn("failed to parse query from %*.s to %*.s bucket %*.s\n", (int)from_len, from, (int)to_len, to, (int)bucket_len,
				 bucket);
		response->status = 400;
		return;
	}

	if (query.from > query.to || query.to - query.from < 1200 || query.to - query.from > 2764800 ||
			(query.to - query.from) / query.bucket > 720) {
		warn("failed to validate query from %lu to %lu bucket %hu\n", query.from, query.to, query.bucket);
		response->status = 400;
		return;
	}

	uint16_t readings_len = 0;
	uint16_t status = reading_select(db, bwt, &query, response, &readings_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hu readings\n", readings_len);
	response->status = 200;
}

void reading_find_by_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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
	if (strnfind(request->search.ptr, request->search.len, "to=", "&", &to, &to_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *bucket;
	size_t bucket_len;
	if (strnfind(request->search.ptr, request->search.len, "bucket=", "", &bucket, &bucket_len, 8) == -1) {
		response->status = 400;
		return;
	}

	reading_query_t query = {.from = 0, .to = 0, .bucket = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1 || strnto64(to, to_len, (uint64_t *)&query.to) == -1 ||
			strnto16(bucket, bucket_len, &query.bucket) == -1) {
		warn("failed to parse query from %*.s to %*.s bucket %*.s\n", (int)from_len, from, (int)to_len, to, (int)bucket_len,
				 bucket);
		response->status = 400;
		return;
	}

	if (query.from > query.to || query.to - query.from < 1200 || query.to - query.from > 2764800 ||
			(query.to - query.from) / query.bucket > 720) {
		warn("failed to validate query from %lu to %lu bucket %hu\n", query.from, query.to, query.bucket);
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

	uint16_t readings_len = 0;
	status = reading_select_by_device(db, &device, &query, response, &readings_len);
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

void reading_find_by_zone(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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
	if (strnfind(request->search.ptr, request->search.len, "to=", "&", &to, &to_len, 32) == -1) {
		response->status = 400;
		return;
	}

	const char *bucket;
	size_t bucket_len;
	if (strnfind(request->search.ptr, request->search.len, "bucket=", "", &bucket, &bucket_len, 8) == -1) {
		response->status = 400;
		return;
	}

	reading_query_t query = {.from = 0, .to = 0, .bucket = 0};
	if (strnto64(from, from_len, (uint64_t *)&query.from) == -1 || strnto64(to, to_len, (uint64_t *)&query.to) == -1 ||
			strnto16(bucket, bucket_len, &query.bucket) == -1) {
		warn("failed to parse query from %*.s to %*.s bucket %*.s\n", (int)from_len, from, (int)to_len, to, (int)bucket_len,
				 bucket);
		response->status = 400;
		return;
	}

	if (query.from > query.to || query.to - query.from < 1200 || query.to - query.from > 2764800 ||
			(query.to - query.from) / query.bucket > 720) {
		warn("failed to validate query from %lu to %lu bucket %hu\n", query.from, query.to, query.bucket);
		response->status = 400;
		return;
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

	uint16_t readings_len = 0;
	status = reading_select_by_zone(db, &zone, &query, response, &readings_len);
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
