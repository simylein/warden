#include "device.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "database.h"
#include "uplink.h"
#include "user-device.h"
#include "user.h"
#include "zone.h"
#include <fcntl.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *device_table = "device";
const char *device_schema = "create table device ("
														"id blob primary key, "
														"name text not null unique, "
														"zone_id blob, "
														"firmware text, "
														"hardware text, "
														"created_at timestamp not null, "
														"updated_at timestamp, "
														"foreign key (zone_id) references zone(id) on delete set null"
														")";

const device_row_t device_row = {
		.id = 0,
		.name_len = 16,
		.name = 17,
		.firmware_len = 33,
		.firmware = 34,
		.hardware_len = 46,
		.hardware = 47,
		.created_at = 59,
		.updated_at_null = 67,
		.updated_at = 68,
		.zone_null = 76,
		.zone_id = 77,
		.zone_name_len = 93,
		.zone_name = 94,
		.zone_color = 106,
		.reading_null = 118,
		.reading_id = 119,
		.reading_temperature = 135,
		.reading_humidity = 137,
		.reading_captured_at = 139,
		.metric_null = 147,
		.metric_id = 148,
		.metric_photovoltaic = 164,
		.metric_battery = 166,
		.metric_captured_at = 168,
		.buffer_null = 176,
		.buffer_id = 177,
		.buffer_delay = 193,
		.buffer_level = 197,
		.buffer_captured_at = 199,
		.size = 207,
};

int device_rowcmp(uint8_t *alpha, uint8_t *bravo, device_query_t *query) {
	if (query->order_len == 2 && memcmp(query->order, "id", query->order_len) == 0) {
		uint64_t id_alpha = octet_uint64_read(alpha, device_row.id);
		uint64_t id_bravo = octet_uint64_read(bravo, device_row.id);
		int result = (id_alpha > id_bravo) - (id_alpha < id_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 4 && memcmp(query->order, "name", query->order_len) == 0) {
		uint8_t name_len_alpha = octet_uint8_read(alpha, device_row.name_len);
		char *name_alpha = octet_text_read(alpha, device_row.name);
		uint8_t name_len_bravo = octet_uint8_read(bravo, device_row.name_len);
		char *name_bravo = octet_text_read(bravo, device_row.name);
		int result = memcmp(name_alpha, name_bravo, name_len_alpha < name_len_bravo ? name_len_alpha : name_len_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 4 && memcmp(query->order, "zone", query->order_len) == 0) {
		uint8_t zone_len_alpha = octet_uint8_read(alpha, device_row.zone_name_len);
		char *zone_alpha = octet_text_read(alpha, device_row.zone_name);
		uint8_t zone_len_bravo = octet_uint8_read(bravo, device_row.zone_name_len);
		char *zone_bravo = octet_text_read(bravo, device_row.zone_name);
		int result = memcmp(zone_alpha, zone_bravo, zone_len_alpha < zone_len_bravo ? zone_len_alpha : zone_len_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 11 && memcmp(query->order, "temperature", query->order_len) == 0) {
		int16_t temperature_alpha = octet_int16_read(alpha, device_row.reading_temperature);
		int16_t temperature_bravo = octet_int16_read(bravo, device_row.reading_temperature);
		int result = (temperature_alpha > temperature_bravo) - (temperature_alpha < temperature_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 8 && memcmp(query->order, "humidity", query->order_len) == 0) {
		uint16_t humidity_alpha = octet_uint16_read(alpha, device_row.reading_humidity);
		uint16_t humidity_bravo = octet_uint16_read(bravo, device_row.reading_humidity);
		int result = (humidity_alpha > humidity_bravo) - (humidity_alpha < humidity_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 12 && memcmp(query->order, "photovoltaic", query->order_len) == 0) {
		uint16_t photovoltaic_alpha = octet_uint16_read(alpha, device_row.metric_photovoltaic);
		uint16_t photovoltaic_bravo = octet_uint16_read(bravo, device_row.metric_photovoltaic);
		int result = (photovoltaic_alpha > photovoltaic_bravo) - (photovoltaic_alpha < photovoltaic_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 7 && memcmp(query->order, "battery", query->order_len) == 0) {
		uint16_t battery_alpha = octet_uint16_read(alpha, device_row.metric_battery);
		uint16_t battery_bravo = octet_uint16_read(bravo, device_row.metric_battery);
		int result = (battery_alpha > battery_bravo) - (battery_alpha < battery_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 5 && memcmp(query->order, "delay", query->order_len) == 0) {
		uint32_t delay_alpha = octet_uint32_read(alpha, device_row.buffer_delay);
		uint32_t delay_bravo = octet_uint32_read(bravo, device_row.buffer_delay);
		int result = (delay_alpha > delay_bravo) - (delay_alpha < delay_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 5 && memcmp(query->order, "level", query->order_len) == 0) {
		uint16_t level_alpha = octet_uint16_read(alpha, device_row.buffer_level);
		uint16_t level_bravo = octet_uint16_read(bravo, device_row.buffer_level);
		int result = (level_alpha > level_bravo) - (level_alpha < level_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	return 0;
}

uint16_t device_existing(octet_t *db, device_t *device) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/device.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("select existing device %02x%02x\n", (*device->id)[0], (*device->id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->buffer, device_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, device_row.id);
		if (memcmp(id, device->id, sizeof(*device->id)) == 0) {
			status = 0;
			break;
		}
		offset += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_select(octet_t *db, bwt_t *bwt, device_query_t *query, response_t *response, uint8_t *devices_len) {
	uint16_t status;
	uint8_t *row = db->buffer;
	uint8_t *table = &db->buffer[device_row.size];

	char file[128];
	if (sprintf(file, "%s/device.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	if (stmt.stat.st_size > db->buffer_len - device_row.size) {
		error("file length %zu exceeds buffer length %u\n", (size_t)stmt.stat.st_size, db->buffer_len - device_row.size);
		status = 500;
		goto cleanup;
	}

	debug("select devices for user %02x%02x order by %.*s:%.*s\n", bwt->id[0], bwt->id[1], (int)query->order_len, query->order,
				(int)query->sort_len, query->sort);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &table[offset], device_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		offset += device_row.size;
	}

	for (uint8_t index = 0; index < stmt.stat.st_size / device_row.size - 1; index++) {
		for (uint8_t ind = index + 1; ind < stmt.stat.st_size / device_row.size; ind++) {
			if (device_rowcmp(&table[index * device_row.size], &table[ind * device_row.size], query) > 0) {
				memcpy(row, &table[index * device_row.size], device_row.size);
				memcpy(&table[index * device_row.size], &table[ind * device_row.size], device_row.size);
				memcpy(&table[ind * device_row.size], row, device_row.size);
			}
		}
	}

	offset = device_row.size * query->offset;
	while (true) {
		if (offset >= stmt.stat.st_size || *devices_len >= query->limit) {
			status = 0;
			break;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&table[offset], device_row.id);
		uint8_t name_len = octet_uint8_read(&table[offset], device_row.name_len);
		char *name = octet_text_read(&table[offset], device_row.name);
		time_t created_at = (time_t)octet_uint64_read(&table[offset], device_row.created_at);
		uint8_t updated_at_null = octet_uint8_read(&table[offset], device_row.updated_at_null);
		time_t updated_at = (time_t)octet_uint64_read(&table[offset], device_row.updated_at);
		uint8_t zone_null = octet_uint8_read(&table[offset], device_row.zone_null);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(&table[offset], device_row.zone_id);
		uint8_t zone_name_len = octet_uint8_read(&table[offset], device_row.zone_name_len);
		char *zone_name = octet_text_read(&table[offset], device_row.zone_name);
		uint8_t (*zone_color)[12] = (uint8_t (*)[12])octet_blob_read(&table[offset], device_row.zone_color);
		uint8_t reading_null = octet_uint8_read(&table[offset], device_row.reading_null);
		uint8_t (*reading_id)[16] = (uint8_t (*)[16])octet_blob_read(&table[offset], device_row.reading_id);
		int16_t reading_temperature = octet_int16_read(&table[offset], device_row.reading_temperature);
		uint16_t reading_humidity = octet_uint16_read(&table[offset], device_row.reading_humidity);
		time_t reading_captured_at = (time_t)octet_uint64_read(&table[offset], device_row.reading_captured_at);
		uint8_t metric_null = octet_uint8_read(&table[offset], device_row.metric_null);
		uint8_t (*metric_id)[16] = (uint8_t (*)[16])octet_blob_read(&table[offset], device_row.metric_id);
		uint16_t metric_photovoltaic = octet_uint16_read(&table[offset], device_row.metric_photovoltaic);
		uint16_t metric_battery = octet_uint16_read(&table[offset], device_row.metric_battery);
		time_t metric_captured_at = (time_t)octet_uint64_read(&table[offset], device_row.metric_captured_at);
		uint8_t buffer_null = octet_uint8_read(&table[offset], device_row.buffer_null);
		uint8_t (*buffer_id)[16] = (uint8_t (*)[16])octet_blob_read(&table[offset], device_row.buffer_id);
		uint32_t buffer_delay = octet_uint32_read(&table[offset], device_row.buffer_delay);
		uint16_t buffer_level = octet_uint16_read(&table[offset], device_row.buffer_level);
		time_t buffer_captured_at = (time_t)octet_uint64_read(&table[offset], device_row.buffer_captured_at);
		body_write(response, id, sizeof(*id));
		body_write(response, name, name_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
		body_write(response, (uint8_t[]){updated_at_null != 0x00}, sizeof(updated_at_null));
		if (updated_at_null != 0x00) {
			body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
		}
		body_write(response, (uint8_t[]){zone_null != 0x00}, sizeof(zone_null));
		if (zone_null != 0x00) {
			body_write(response, zone_id, sizeof(*zone_id));
			body_write(response, zone_name, zone_name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, zone_color, sizeof(*zone_color));
		}
		body_write(response, (uint8_t[]){reading_null != 0x00}, sizeof(reading_null));
		if (reading_null != 0x00) {
			body_write(response, reading_id, sizeof(*reading_id));
			body_write(response, (uint16_t[]){hton16((uint16_t)reading_temperature)}, sizeof(reading_temperature));
			body_write(response, (uint16_t[]){hton16(reading_humidity)}, sizeof(reading_humidity));
			body_write(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
		}
		body_write(response, (uint8_t[]){metric_null != 0x00}, sizeof(metric_null));
		if (metric_null != 0x00) {
			body_write(response, metric_id, sizeof(*metric_id));
			body_write(response, (uint16_t[]){hton16(metric_photovoltaic)}, sizeof(metric_photovoltaic));
			body_write(response, (uint16_t[]){hton16(metric_battery)}, sizeof(metric_battery));
			body_write(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
		}
		body_write(response, (uint8_t[]){buffer_null != 0x00}, sizeof(buffer_null));
		if (buffer_null != 0x00) {
			body_write(response, buffer_id, sizeof(*buffer_id));
			body_write(response, (uint32_t[]){hton32(buffer_delay)}, sizeof(buffer_delay));
			body_write(response, (uint16_t[]){hton16(buffer_level)}, sizeof(buffer_level));
			body_write(response, (uint64_t[]){hton64((uint64_t)buffer_captured_at)}, sizeof(buffer_captured_at));
		}
		cache_device_t cache_device;
		memcpy(cache_device.id, id, sizeof(cache_device.id));
		memcpy(cache_device.name, name, name_len);
		cache_device.name_len = name_len;
		if (zone_null != 0x00) {
			memcpy(cache_device.zone_name, zone_name, zone_name_len);
			cache_device.zone_name_len = zone_name_len;
		} else {
			cache_device.zone_name_len = 0;
		}
		if (cache_device_write(&cache_device) == -1) {
			warn("failed to cache device %02x%02x\n", (*id)[0], (*id)[1]);
		}
		*devices_len += 1;
		offset += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_select_one(octet_t *db, bwt_t *bwt, device_t *device, response_t *response) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/device.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("select device %02x%02x for user %02x%02x\n", (*device->id)[0], (*device->id)[1], bwt->id[0], bwt->id[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->buffer, device_row.size) == -1) {
			status = 500;
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, device_row.id);
		if (memcmp(id, device->id, sizeof(*device->id)) == 0) {
			uint8_t name_len = octet_uint8_read(db->buffer, device_row.name_len);
			char *name = octet_text_read(db->buffer, device_row.name);
			uint8_t firmware_len = octet_uint8_read(db->buffer, device_row.firmware_len);
			char *firmware = octet_text_read(db->buffer, device_row.firmware);
			uint8_t hardware_len = octet_uint8_read(db->buffer, device_row.hardware_len);
			char *hardware = octet_text_read(db->buffer, device_row.hardware);
			time_t created_at = (time_t)octet_uint64_read(db->buffer, device_row.created_at);
			uint8_t updated_at_null = octet_uint8_read(db->buffer, device_row.updated_at_null);
			time_t updated_at = (time_t)octet_uint64_read(db->buffer, device_row.updated_at);
			uint8_t zone_null = octet_uint8_read(db->buffer, device_row.zone_null);
			uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, device_row.zone_id);
			uint8_t zone_name_len = octet_uint8_read(db->buffer, device_row.zone_name_len);
			char *zone_name = octet_text_read(db->buffer, device_row.zone_name);
			uint8_t (*zone_color)[12] = (uint8_t (*)[12])octet_blob_read(db->buffer, device_row.zone_color);
			uint8_t reading_null = octet_uint8_read(db->buffer, device_row.reading_null);
			uint8_t (*reading_id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, device_row.reading_id);
			int16_t reading_temperature = octet_int16_read(db->buffer, device_row.reading_temperature);
			uint16_t reading_humidity = octet_uint16_read(db->buffer, device_row.reading_humidity);
			time_t reading_captured_at = (time_t)octet_uint64_read(db->buffer, device_row.reading_captured_at);
			uint8_t metric_null = octet_uint8_read(db->buffer, device_row.metric_null);
			uint8_t (*metric_id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, device_row.metric_id);
			uint16_t metric_photovoltaic = octet_uint16_read(db->buffer, device_row.metric_photovoltaic);
			uint16_t metric_battery = octet_uint16_read(db->buffer, device_row.metric_battery);
			time_t metric_captured_at = (time_t)octet_uint64_read(db->buffer, device_row.metric_captured_at);
			uint8_t buffer_null = octet_uint8_read(db->buffer, device_row.buffer_null);
			uint8_t (*buffer_id)[16] = (uint8_t (*)[16])octet_blob_read(db->buffer, device_row.buffer_id);
			uint32_t buffer_delay = octet_uint32_read(db->buffer, device_row.buffer_delay);
			uint16_t buffer_level = octet_uint16_read(db->buffer, device_row.buffer_level);
			time_t buffer_captured_at = (time_t)octet_uint64_read(db->buffer, device_row.buffer_captured_at);
			uint8_t uplink_null = 0x00;
			uint8_t downlink_null = 0x00;
			body_write(response, id, sizeof(*id));
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint8_t[]){firmware_len != 0}, sizeof(firmware_len));
			if (firmware_len != 0) {
				body_write(response, firmware, firmware_len);
				body_write(response, (char[]){0x00}, sizeof(char));
			}
			body_write(response, (uint8_t[]){hardware_len != 0}, sizeof(hardware_len));
			if (hardware_len != 0) {
				body_write(response, hardware, hardware_len);
				body_write(response, (char[]){0x00}, sizeof(char));
			}
			body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
			body_write(response, (uint8_t[]){updated_at_null != 0x00}, sizeof(updated_at_null));
			if (updated_at_null != 0x00) {
				body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
			}
			body_write(response, (uint8_t[]){zone_null != 0x00}, sizeof(zone_null));
			if (zone_null != 0x00) {
				body_write(response, zone_id, sizeof(*zone_id));
				body_write(response, zone_name, zone_name_len);
				body_write(response, (char[]){0x00}, sizeof(char));
				body_write(response, zone_color, sizeof(*zone_color));
			}
			body_write(response, (uint8_t[]){reading_null != 0x00}, sizeof(reading_null));
			if (reading_null != 0x00) {
				body_write(response, reading_id, sizeof(*reading_id));
				body_write(response, (uint16_t[]){hton16((uint16_t)reading_temperature)}, sizeof(reading_temperature));
				body_write(response, (uint16_t[]){hton16(reading_humidity)}, sizeof(reading_humidity));
				body_write(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
			}
			body_write(response, (uint8_t[]){metric_null != 0x00}, sizeof(metric_null));
			if (metric_null != 0x00) {
				body_write(response, metric_id, sizeof(*metric_id));
				body_write(response, (uint16_t[]){hton16(metric_photovoltaic)}, sizeof(metric_photovoltaic));
				body_write(response, (uint16_t[]){hton16(metric_battery)}, sizeof(metric_battery));
				body_write(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
			}
			body_write(response, (uint8_t[]){buffer_null != 0x00}, sizeof(buffer_null));
			if (buffer_null != 0x00) {
				body_write(response, buffer_id, sizeof(*buffer_id));
				body_write(response, (uint32_t[]){hton32(buffer_delay)}, sizeof(buffer_delay));
				body_write(response, (uint16_t[]){hton16(buffer_level)}, sizeof(buffer_level));
				body_write(response, (uint64_t[]){hton64((uint64_t)buffer_captured_at)}, sizeof(buffer_captured_at));
			}
			body_write(response, (uint8_t[]){uplink_null != 0x00}, sizeof(uplink_null));
			body_write(response, (uint8_t[]){downlink_null != 0x00}, sizeof(downlink_null));
			cache_device_t cache_device;
			memcpy(cache_device.id, id, sizeof(cache_device.id));
			memcpy(cache_device.name, name, name_len);
			cache_device.name_len = name_len;
			if (zone_null != 0x00) {
				memcpy(cache_device.zone_name, zone_name, zone_name_len);
				cache_device.zone_name_len = zone_name_len;
			} else {
				cache_device.zone_name_len = 0;
			}
			if (cache_device_write(&cache_device) == -1) {
				warn("failed to cache device %02x%02x\n", (*id)[0], (*id)[1]);
			}
			status = 0;
			break;
		}
		offset += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_select_by_user(sqlite3 *database, user_t *user, device_query_t *query, response_t *response,
															 uint8_t *devices_len) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "select "
										"device.id, device.name, device.created_at, device.updated_at, "
										"zone.id, zone.name, zone.color, "
										"uplink.id, uplink.received_at "
										"from device "
										"join user_device on user_device.device_id = device.id and user_device.user_id = ?1 "
										"left join zone on zone.id = device.zone_id "
										"left join uplink on uplink.id = "
										"(select id from uplink where device_id = device.id order by uplink.received_at desc limit 1) "
										"order by "
										"case when ?2 = 'id' and ?3 = 'asc' then device.id end asc, "
										"case when ?2 = 'id' and ?3 = 'desc' then device.id end desc, "
										"case when ?2 = 'name' and ?3 = 'asc' then device.name end asc, "
										"case when ?2 = 'name' and ?3 = 'desc' then device.name end desc, "
										"case when ?2 = 'zone' and ?3 = 'asc' then zone.name end asc, "
										"case when ?2 = 'zone' and ?3 = 'desc' then zone.name end desc, "
										"case when ?2 = 'createdAt' and ?3 = 'asc' then device.created_at end asc, "
										"case when ?2 = 'createdAt' and ?3 = 'desc' then device.created_at end desc, "
										"case when ?2 = 'updatedAt' and ?3 = 'asc' then device.updated_at end asc, "
										"case when ?2 = 'updatedAt' and ?3 = 'desc' then device.updated_at end desc, "
										"case when ?2 = 'receivedAt' and ?3 = 'asc' then uplink.received_at end asc, "
										"case when ?2 = 'receivedAt' and ?3 = 'desc' then uplink.received_at end desc "
										"limit ?4 offset ?5";
	debug("select devices for user %02x%02x order by %.*s:%.*s\n", (*user->id)[0], (*user->id)[1], (int)query->order_len,
				query->order, (int)query->sort_len, query->sort);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	sqlite3_bind_blob(stmt, 1, user->id, sizeof(*user->id), SQLITE_STATIC);
	sqlite3_bind_text(stmt, 2, query->order, (uint8_t)query->order_len, SQLITE_STATIC);
	sqlite3_bind_text(stmt, 3, query->sort, (uint8_t)query->sort_len, SQLITE_STATIC);
	sqlite3_bind_int(stmt, 4, query->limit);
	sqlite3_bind_int64(stmt, 5, query->offset);

	while (true) {
		int result = sqlite3_step(stmt);
		if (result == SQLITE_ROW) {
			const uint8_t *id = sqlite3_column_blob(stmt, 0);
			const size_t id_len = (size_t)sqlite3_column_bytes(stmt, 0);
			if (id_len != sizeof(*((device_t *)0)->id)) {
				error("id length %zu does not match buffer length %zu\n", id_len, sizeof(*((device_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *name = sqlite3_column_text(stmt, 1);
			const size_t name_len = (size_t)sqlite3_column_bytes(stmt, 1);
			const time_t created_at = (time_t)sqlite3_column_int64(stmt, 2);
			const time_t updated_at = (time_t)sqlite3_column_int64(stmt, 3);
			const int updated_at_type = sqlite3_column_type(stmt, 3);
			const uint8_t *zone_id = sqlite3_column_blob(stmt, 4);
			const size_t zone_id_len = (size_t)sqlite3_column_bytes(stmt, 4);
			const int zone_id_type = sqlite3_column_type(stmt, 4);
			if (zone_id_type != SQLITE_NULL && zone_id_len != sizeof(*((zone_t *)0)->id)) {
				error("zone id length %zu does not match buffer length %zu\n", zone_id_len, sizeof(*((zone_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const uint8_t *zone_name = sqlite3_column_text(stmt, 5);
			const size_t zone_name_len = (size_t)sqlite3_column_bytes(stmt, 5);
			const int zone_name_type = sqlite3_column_type(stmt, 5);
			const uint8_t *zone_color = sqlite3_column_blob(stmt, 6);
			const size_t zone_color_len = (size_t)sqlite3_column_bytes(stmt, 6);
			const int zone_color_type = sqlite3_column_type(stmt, 6);
			if (zone_color_type != SQLITE_NULL && zone_color_len != sizeof(*((zone_t *)0)->color)) {
				error("zone color length %zu does not match buffer length %zu\n", zone_color_len, sizeof(*((zone_t *)0)->color));
				status = 500;
				goto cleanup;
			}
			const uint8_t *uplink_id = sqlite3_column_blob(stmt, 7);
			const size_t uplink_id_len = (size_t)sqlite3_column_bytes(stmt, 7);
			const int uplink_id_type = sqlite3_column_type(stmt, 7);
			if (uplink_id_type != SQLITE_NULL && uplink_id_len != sizeof(*((uplink_t *)0)->id)) {
				error("uplink id length %zu does not match buffer length %zu\n", uplink_id_len, sizeof(*((uplink_t *)0)->id));
				status = 500;
				goto cleanup;
			}
			const time_t uplink_received_at = (time_t)sqlite3_column_int64(stmt, 8);
			const int uplink_received_at_type = sqlite3_column_type(stmt, 8);
			body_write(response, id, id_len);
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
			body_write(response, (char[]){updated_at_type != SQLITE_NULL}, sizeof(char));
			if (updated_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
			}
			body_write(response, (char[]){zone_id_type != SQLITE_NULL}, sizeof(char));
			if (zone_id_type != SQLITE_NULL) {
				body_write(response, zone_id, zone_id_len);
			}
			body_write(response, (char[]){zone_name_type != SQLITE_NULL}, sizeof(char));
			if (zone_name_type != SQLITE_NULL) {
				body_write(response, zone_name, zone_name_len);
				body_write(response, (char[]){0x00}, sizeof(char));
			}
			body_write(response, (char[]){zone_color_type != SQLITE_NULL}, sizeof(char));
			if (zone_color_type != SQLITE_NULL) {
				body_write(response, zone_color, zone_color_len);
			}
			body_write(response, (char[]){uplink_id_type != SQLITE_NULL}, sizeof(char));
			if (uplink_id_type != SQLITE_NULL) {
				body_write(response, uplink_id, uplink_id_len);
			}
			body_write(response, (char[]){uplink_received_at_type != SQLITE_NULL}, sizeof(char));
			if (uplink_received_at_type != SQLITE_NULL) {
				body_write(response, (uint64_t[]){hton64((uint64_t)uplink_received_at)}, sizeof(uplink_received_at));
			}
			*devices_len += 1;
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

int device_parse(device_t *device, request_t *request) {
	request->body.pos = 0;

	uint8_t stage = 0;

	device->name_len = 0;
	const uint8_t name_index = (uint8_t)request->body.pos;
	while (stage == 0 && device->name_len < 16 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 1;
		} else {
			device->name_len++;
		}
	}
	if (device->name_len != 0) {
		device->name = &request->body.ptr[name_index];
	} else {
		device->name = NULL;
	}
	if (stage != 1) {
		debug("found name with %hhu bytes\n", device->name_len);
		return -1;
	}

	if (request->body.len < request->body.pos + sizeof(*device->zone_id)) {
		debug("missing zone id on device\n");
		return -1;
	}
	device->zone_id = (uint8_t (*)[16])body_read(request, sizeof(*device->zone_id));
	stage = 2;

	device->firmware_len = 0;
	const uint8_t firmware_index = (uint8_t)request->body.pos;
	while (stage == 2 && device->firmware_len < 12 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 3;
		} else {
			device->firmware_len++;
		}
	}
	if (device->firmware_len != 0) {
		device->firmware = &request->body.ptr[firmware_index];
	} else {
		device->firmware = NULL;
	}
	if (stage != 3) {
		debug("found firmware with %hhu bytes\n", device->firmware_len);
		return -1;
	}

	device->hardware_len = 0;
	const uint8_t hardware_index = (uint8_t)request->body.pos;
	while (stage == 3 && device->hardware_len < 12 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 4;
		} else {
			device->hardware_len++;
		}
	}
	if (device->hardware_len != 0) {
		device->hardware = &request->body.ptr[hardware_index];
	} else {
		device->hardware = NULL;
	}
	if (stage != 4) {
		debug("found hardware with %hhu bytes\n", device->hardware_len);
		return -1;
	}

	return 0;
}

int device_validate(device_t *device) {
	if (device->name != NULL) {
		if (device->name_len < 2) {
			return -1;
		}

		uint8_t name_index = 0;
		while (name_index < device->name_len) {
			char *byte = &device->name[name_index];
			if ((*byte < 'a' || *byte > 'z') && (*byte < '0' || *byte > '9') && *byte != ' ') {
				debug("name contains invalid character %02x\n", *byte);
				return -1;
			}
			name_index++;
		}
	}

	if (device->firmware != NULL) {
		if (device->firmware_len < 4) {
			return -1;
		}

		uint8_t firmware_index = 0;
		while (firmware_index < device->firmware_len) {
			char *byte = &device->firmware[firmware_index];
			if ((*byte < '0' || *byte > '9') && *byte != '.' && *byte != '-' && *byte != 'r' && *byte != 'c') {
				debug("firmware contains invalid character %02x\n", *byte);
				return -1;
			}
			firmware_index++;
		}
	}

	if (device->hardware != NULL) {
		if (device->hardware_len < 4) {
			return -1;
		}

		uint8_t hardware_index = 0;
		while (hardware_index < device->hardware_len) {
			char *byte = &device->hardware[hardware_index];
			if ((*byte < '0' || *byte > '9') && *byte != '.' && *byte != '-' && *byte != 'r' && *byte != 'c') {
				debug("hardware contains invalid character %02x\n", *byte);
				return -1;
			}
			hardware_index++;
		}
	}

	return 0;
}

uint16_t device_insert(octet_t *db, device_t *device) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*device->id); index++) {
		(*device->id)[index] = (uint8_t)(rand() % 0xff);
	}

	char file[128];
	if (sprintf(file, "%s/device.data", db->directory) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = 500;
		goto cleanup;
	}

	debug("insert device name %*.s created at %lu\n", device->name_len, device->name, *device->created_at);

	octet_blob_write(db->buffer, device_row.id, (uint8_t *)device->id, sizeof(*device->id));
	octet_uint8_write(db->buffer, device_row.name_len, device->name_len);
	octet_text_write(db->buffer, device_row.name, (char *)device->name, device->name_len);
	if (device->zone_id != NULL) {
		octet_uint8_write(db->buffer, device_row.zone_null, 0x01);
		octet_blob_write(db->buffer, device_row.zone_id, (uint8_t *)device->zone_id, sizeof(*device->zone_id));
		octet_uint8_write(db->buffer, device_row.zone_name_len, device->zone_name_len);
		octet_text_write(db->buffer, device_row.zone_name, (char *)device->zone_name, device->zone_name_len);
		octet_blob_write(db->buffer, device_row.zone_color, (uint8_t *)device->zone_color, sizeof(*device->zone_color));
	} else {
		octet_uint8_write(db->buffer, device_row.zone_null, 0x00);
	}
	if (device->firmware != NULL) {
		octet_uint8_write(db->buffer, device_row.firmware_len, device->firmware_len);
		octet_text_write(db->buffer, device_row.firmware, device->firmware, device->firmware_len);
	} else {
		octet_uint8_write(db->buffer, device_row.firmware_len, 0);
	}
	if (device->hardware != NULL) {
		octet_uint8_write(db->buffer, device_row.hardware_len, device->hardware_len);
		octet_text_write(db->buffer, device_row.hardware, device->hardware, device->hardware_len);
	} else {
		octet_uint8_write(db->buffer, device_row.hardware_len, 0);
	}
	octet_uint64_write(db->buffer, device_row.created_at, (uint64_t)*device->created_at);
	octet_uint8_write(db->buffer, device_row.updated_at_null, 0x00);
	octet_uint8_write(db->buffer, device_row.reading_null, 0x00);
	octet_uint8_write(db->buffer, device_row.metric_null, 0x00);
	octet_uint8_write(db->buffer, device_row.buffer_null, 0x00);

	off_t offset = stmt.stat.st_size;
	if (octet_row_write(&stmt, file, offset, db->buffer, device_row.size) == -1) {
		status = 500;
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_update(sqlite3 *database, device_t *device) {
	uint16_t status;
	sqlite3_stmt *stmt;

	const char *sql = "update device "
										"set name = coalesce(?, name), zone_id = coalesce(?, zone_id), "
										"firmware = coalesce(?, firmware), hardware = coalesce(?, hardware), "
										"updated_at = ? "
										"where id = ?";
	debug("%s\n", sql);

	if (sqlite3_prepare_v2(database, sql, -1, &stmt, NULL) != SQLITE_OK) {
		error("failed to prepare statement because %s\n", sqlite3_errmsg(database));
		status = 500;
		goto cleanup;
	}

	if (device->name != NULL) {
		sqlite3_bind_text(stmt, 1, device->name, device->name_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 1);
	}
	if (device->zone_id != NULL) {
		sqlite3_bind_blob(stmt, 2, device->zone_id, sizeof(*device->zone_id), SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 2);
	}
	if (device->firmware != NULL) {
		sqlite3_bind_text(stmt, 3, device->firmware, device->firmware_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 3);
	}
	if (device->hardware != NULL) {
		sqlite3_bind_text(stmt, 4, device->hardware, device->hardware_len, SQLITE_STATIC);
	} else {
		sqlite3_bind_null(stmt, 4);
	}
	sqlite3_bind_int64(stmt, 5, *device->updated_at);
	sqlite3_bind_blob(stmt, 6, *device->id, sizeof(*device->id), SQLITE_STATIC);

	int result = sqlite3_step(stmt);
	if (result != SQLITE_DONE) {
		status = database_error(database, result);
		goto cleanup;
	}

	if (sqlite3_changes(database) == 0) {
		warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
		status = 404;
		goto cleanup;
	}

	status = 0;

cleanup:
	sqlite3_finalize(stmt);
	return status;
}

void device_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	device_query_t query = {.limit = 16, .offset = 0};
	if (strnfind(request->search.ptr, request->search.len, "order=", "&", &query.order, &query.order_len, 16) == -1) {
		response->status = 400;
		return;
	}

	if (strnfind(request->search.ptr, request->search.len, "sort=", "", &query.sort, &query.sort_len, 8) == -1) {
		response->status = 400;
		return;
	}

	uint8_t devices_len = 0;
	uint16_t status = device_select(db, bwt, &query, response, &devices_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu devices\n", devices_len);
	response->status = 200;
}

void device_find_one(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
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

	status = device_select_one(db, bwt, &device, response);
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
	info("found device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}

void device_find_by_user(octet_t *db, sqlite3 *database, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = param_find(request, 10, &uuid_len);
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

	device_query_t query = {.limit = 16, .offset = 0};
	if (strnfind(request->search.ptr, request->search.len, "order=", "&", &query.order, &query.order_len, 16) == -1) {
		response->status = 400;
		return;
	}

	if (strnfind(request->search.ptr, request->search.len, "sort=", "", &query.sort, &query.sort_len, 8) == -1) {
		response->status = 400;
		return;
	}

	user_t user = {.id = &id};
	uint16_t status = user_existing(db, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	uint8_t devices_len = 0;
	status = device_select_by_user(database, &user, &query, response, &devices_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu devices\n", devices_len);
	response->status = 200;
}

void device_modify(sqlite3 *database, request_t *request, response_t *response) {
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

	time_t now = time(NULL);
	device_t device = {.id = &id, .updated_at = &now};
	if (request->body.len == 0 || device_parse(&device, request) == -1 || device_validate(&device) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = device_update(database, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("updated device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}
