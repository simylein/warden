#include "device.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "buffer.h"
#include "cache.h"
#include "config.h"
#include "downlink.h"
#include "metric.h"
#include "radio.h"
#include "reading.h"
#include "uplink.h"
#include "user-device.h"
#include "user.h"
#include "zone.h"
#include <fcntl.h>
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

const char *device_file = "device";

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

	if (query->order_len == 9 && memcmp(query->order, "createdAt", query->order_len) == 0) {
		time_t created_at_alpha = (time_t)octet_uint64_read(alpha, device_row.created_at);
		time_t created_at_bravo = (time_t)octet_uint64_read(bravo, device_row.created_at);
		int result = (created_at_alpha > created_at_bravo) - (created_at_alpha < created_at_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 9 && memcmp(query->order, "updatedAt", query->order_len) == 0) {
		time_t updated_at_alpha = (time_t)octet_uint64_read(alpha, device_row.updated_at);
		time_t updated_at_bravo = (time_t)octet_uint64_read(bravo, device_row.updated_at);
		int result = (updated_at_alpha > updated_at_bravo) - (updated_at_alpha < updated_at_bravo);
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
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
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
		if (octet_row_read(&stmt, file, offset, db->row, device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.id);
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

	uint8_t user_devices_len = 0;
	user_t user = {.id = &bwt->id};
	status = user_device_select_by_user(db, &user, &user_devices_len);
	if (status != 0) {
		return status;
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	if (stmt.stat.st_size > db->table_len) {
		error("file length %zu exceeds buffer length %u\n", (size_t)stmt.stat.st_size, db->table_len);
		status = 500;
		goto cleanup;
	}

	debug("select devices for user %02x%02x order by %.*s:%.*s\n", bwt->id[0], bwt->id[1], (int)query->order_len, query->order,
				(int)query->sort_len, query->sort);

	off_t offset = 0;
	uint32_t table_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->table[table_len], device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[table_len], device_row.id);
		for (uint8_t index = 0; index < user_devices_len; index++) {
			uint8_t (*device_id)[16] =
					(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);
			if (memcmp(id, device_id, sizeof(*device_id)) == 0) {
				table_len += device_row.size;
				break;
			}
		}
		offset += device_row.size;
	}

	if (table_len >= device_row.size * 2) {
		for (uint8_t index = 0; index < table_len / device_row.size - 1; index++) {
			for (uint8_t ind = index + 1; ind < table_len / device_row.size; ind++) {
				if (device_rowcmp(&db->table[index * device_row.size], &db->table[ind * device_row.size], query) > 0) {
					memcpy(db->row, &db->table[index * device_row.size], device_row.size);
					memcpy(&db->table[index * device_row.size], &db->table[ind * device_row.size], device_row.size);
					memcpy(&db->table[ind * device_row.size], db->row, device_row.size);
				}
			}
		}
	}

	uint32_t index = device_row.size * query->offset;
	while (true) {
		if (index >= table_len || *devices_len >= query->limit) {
			status = 0;
			break;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.id);
		uint8_t name_len = octet_uint8_read(&db->table[index], device_row.name_len);
		char *name = octet_text_read(&db->table[index], device_row.name);
		time_t created_at = (time_t)octet_uint64_read(&db->table[index], device_row.created_at);
		uint8_t updated_at_null = octet_uint8_read(&db->table[index], device_row.updated_at_null);
		time_t updated_at = (time_t)octet_uint64_read(&db->table[index], device_row.updated_at);
		uint8_t zone_null = octet_uint8_read(&db->table[index], device_row.zone_null);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.zone_id);
		uint8_t zone_name_len = octet_uint8_read(&db->table[index], device_row.zone_name_len);
		char *zone_name = octet_text_read(&db->table[index], device_row.zone_name);
		uint8_t (*zone_color)[12] = (uint8_t (*)[12])octet_blob_read(&db->table[index], device_row.zone_color);
		uint8_t reading_null = octet_uint8_read(&db->table[index], device_row.reading_null);
		uint8_t (*reading_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.reading_id);
		int16_t reading_temperature = octet_int16_read(&db->table[index], device_row.reading_temperature);
		uint16_t reading_humidity = octet_uint16_read(&db->table[index], device_row.reading_humidity);
		time_t reading_captured_at = (time_t)octet_uint64_read(&db->table[index], device_row.reading_captured_at);
		uint8_t metric_null = octet_uint8_read(&db->table[index], device_row.metric_null);
		uint8_t (*metric_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.metric_id);
		uint16_t metric_photovoltaic = octet_uint16_read(&db->table[index], device_row.metric_photovoltaic);
		uint16_t metric_battery = octet_uint16_read(&db->table[index], device_row.metric_battery);
		time_t metric_captured_at = (time_t)octet_uint64_read(&db->table[index], device_row.metric_captured_at);
		uint8_t buffer_null = octet_uint8_read(&db->table[index], device_row.buffer_null);
		uint8_t (*buffer_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.buffer_id);
		uint32_t buffer_delay = octet_uint32_read(&db->table[index], device_row.buffer_delay);
		uint16_t buffer_level = octet_uint16_read(&db->table[index], device_row.buffer_level);
		time_t buffer_captured_at = (time_t)octet_uint64_read(&db->table[index], device_row.buffer_captured_at);
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
		index += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_select_one(octet_t *db, bwt_t *bwt, device_t *device, response_t *response) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select device %02x%02x for user %02x%02x\n", (*device->id)[0], (*device->id)[1], bwt->id[0], bwt->id[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.id);
		if (memcmp(id, device->id, sizeof(*device->id)) == 0) {
			uint8_t name_len = octet_uint8_read(db->row, device_row.name_len);
			char *name = octet_text_read(db->row, device_row.name);
			uint8_t firmware_len = octet_uint8_read(db->row, device_row.firmware_len);
			char *firmware = octet_text_read(db->row, device_row.firmware);
			uint8_t hardware_len = octet_uint8_read(db->row, device_row.hardware_len);
			char *hardware = octet_text_read(db->row, device_row.hardware);
			time_t created_at = (time_t)octet_uint64_read(db->row, device_row.created_at);
			uint8_t updated_at_null = octet_uint8_read(db->row, device_row.updated_at_null);
			time_t updated_at = (time_t)octet_uint64_read(db->row, device_row.updated_at);
			uint8_t zone_null = octet_uint8_read(db->row, device_row.zone_null);
			uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.zone_id);
			uint8_t zone_name_len = octet_uint8_read(db->row, device_row.zone_name_len);
			char *zone_name = octet_text_read(db->row, device_row.zone_name);
			uint8_t (*zone_color)[12] = (uint8_t (*)[12])octet_blob_read(db->row, device_row.zone_color);
			uint8_t reading_null = octet_uint8_read(db->row, device_row.reading_null);
			uint8_t (*reading_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.reading_id);
			int16_t reading_temperature = octet_int16_read(db->row, device_row.reading_temperature);
			uint16_t reading_humidity = octet_uint16_read(db->row, device_row.reading_humidity);
			time_t reading_captured_at = (time_t)octet_uint64_read(db->row, device_row.reading_captured_at);
			uint8_t metric_null = octet_uint8_read(db->row, device_row.metric_null);
			uint8_t (*metric_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.metric_id);
			uint16_t metric_photovoltaic = octet_uint16_read(db->row, device_row.metric_photovoltaic);
			uint16_t metric_battery = octet_uint16_read(db->row, device_row.metric_battery);
			time_t metric_captured_at = (time_t)octet_uint64_read(db->row, device_row.metric_captured_at);
			uint8_t buffer_null = octet_uint8_read(db->row, device_row.buffer_null);
			uint8_t (*buffer_id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.buffer_id);
			uint32_t buffer_delay = octet_uint32_read(db->row, device_row.buffer_delay);
			uint16_t buffer_level = octet_uint16_read(db->row, device_row.buffer_level);
			time_t buffer_captured_at = (time_t)octet_uint64_read(db->row, device_row.buffer_captured_at);
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

uint16_t device_select_by_user(octet_t *db, user_t *user, device_query_t *query, response_t *response, uint8_t *devices_len) {
	uint16_t status;

	uint8_t user_devices_len = 0;
	status = user_device_select_by_user(db, user, &user_devices_len);
	if (status != 0) {
		return status;
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	if (stmt.stat.st_size > db->table_len) {
		error("file length %zu exceeds buffer length %u\n", (size_t)stmt.stat.st_size, db->table_len);
		status = 500;
		goto cleanup;
	}

	debug("select devices for user %02x%02x order by %.*s:%.*s\n", (*user->id)[0], (*user->id)[1], (int)query->order_len,
				query->order, (int)query->sort_len, query->sort);

	off_t offset = 0;
	uint32_t table_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->table[table_len], device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[table_len], device_row.id);
		for (uint8_t index = 0; index < user_devices_len; index++) {
			uint8_t (*device_id)[16] =
					(uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_device_row.size], user_device_row.device_id);
			if (memcmp(id, device_id, sizeof(*device_id)) == 0) {
				table_len += device_row.size;
				break;
			}
		}
		offset += device_row.size;
	}

	if (table_len >= device_row.size * 2) {
		for (uint8_t index = 0; index < table_len / device_row.size - 1; index++) {
			for (uint8_t ind = index + 1; ind < table_len / device_row.size; ind++) {
				if (device_rowcmp(&db->table[index * device_row.size], &db->table[ind * device_row.size], query) > 0) {
					memcpy(db->row, &db->table[index * device_row.size], device_row.size);
					memcpy(&db->table[index * device_row.size], &db->table[ind * device_row.size], device_row.size);
					memcpy(&db->table[ind * device_row.size], db->row, device_row.size);
				}
			}
		}
	}

	uint32_t index = device_row.size * query->offset;
	while (true) {
		if (index >= table_len || *devices_len >= query->limit) {
			status = 0;
			break;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.id);
		uint8_t name_len = octet_uint8_read(&db->table[index], device_row.name_len);
		char *name = octet_text_read(&db->table[index], device_row.name);
		time_t created_at = (time_t)octet_uint64_read(&db->table[index], device_row.created_at);
		uint8_t updated_at_null = octet_uint8_read(&db->table[index], device_row.updated_at_null);
		time_t updated_at = (time_t)octet_uint64_read(&db->table[index], device_row.updated_at);
		uint8_t zone_null = octet_uint8_read(&db->table[index], device_row.zone_null);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], device_row.zone_id);
		uint8_t zone_name_len = octet_uint8_read(&db->table[index], device_row.zone_name_len);
		char *zone_name = octet_text_read(&db->table[index], device_row.zone_name);
		uint8_t (*zone_color)[12] = (uint8_t (*)[12])octet_blob_read(&db->table[index], device_row.zone_color);
		uint8_t uplink_null = 0x00;
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
		body_write(response, (uint8_t[]){uplink_null != 0x00}, sizeof(uplink_null));
		*devices_len += 1;
		index += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_select_by_zone(octet_t *db, zone_t *zone, uint8_t *devices_len) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select devices for zone %02x%02x\n", (*zone->id)[0], (*zone->id)[1]);

	off_t offset = 0;
	uint16_t chunk_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->chunk[chunk_len], device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t zone_null = octet_uint8_read(&db->chunk[chunk_len], device_row.zone_null);
		uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->chunk[chunk_len], device_row.zone_id);
		if (zone_null != 0x00 && memcmp(zone_id, zone->id, sizeof(*zone->id)) == 0) {
			*devices_len += 1;
			chunk_len += device_row.size;
		}
		offset += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
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
		(*device->id)[index] = (uint8_t)(rand() & 0xff);
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	char uuid[32];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return 500;
	}

	char directory[128];
	if (sprintf(directory, "%s/%.*s", db->directory, (int)sizeof(uuid), uuid) == -1) {
		error("failed to sprintf uuid to directory\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert device name %*.s created at %lu\n", device->name_len, device->name, *device->created_at);

	off_t offset = stmt.stat.st_size;
	while (offset > 0) {
		if (octet_row_read(&stmt, file, offset - device_row.size, db->row, device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint64_t id = octet_uint64_read(db->row, device_row.id);
		uint64_t device_id = octet_uint64_read((uint8_t *)device->id, 0);
		if (id <= device_id) {
			break;
		}
		if (octet_row_write(&stmt, file, offset, db->row, device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		offset -= device_row.size;
	}

	octet_blob_write(db->row, device_row.id, (uint8_t *)device->id, sizeof(*device->id));
	octet_uint8_write(db->row, device_row.name_len, device->name_len);
	octet_text_write(db->row, device_row.name, (char *)device->name, device->name_len);
	if (device->zone_id != NULL) {
		octet_uint8_write(db->row, device_row.zone_null, 0x01);
		octet_blob_write(db->row, device_row.zone_id, (uint8_t *)device->zone_id, sizeof(*device->zone_id));
		octet_uint8_write(db->row, device_row.zone_name_len, device->zone_name_len);
		octet_text_write(db->row, device_row.zone_name, (char *)device->zone_name, device->zone_name_len);
		octet_blob_write(db->row, device_row.zone_color, (uint8_t *)device->zone_color, sizeof(*device->zone_color));
	} else {
		octet_uint8_write(db->row, device_row.zone_null, 0x00);
	}
	if (device->firmware != NULL) {
		octet_uint8_write(db->row, device_row.firmware_len, device->firmware_len);
		octet_text_write(db->row, device_row.firmware, device->firmware, device->firmware_len);
	} else {
		octet_uint8_write(db->row, device_row.firmware_len, 0);
	}
	if (device->hardware != NULL) {
		octet_uint8_write(db->row, device_row.hardware_len, device->hardware_len);
		octet_text_write(db->row, device_row.hardware, device->hardware, device->hardware_len);
	} else {
		octet_uint8_write(db->row, device_row.hardware_len, 0);
	}
	octet_uint64_write(db->row, device_row.created_at, (uint64_t)*device->created_at);
	octet_uint8_write(db->row, device_row.updated_at_null, 0x00);
	octet_uint8_write(db->row, device_row.reading_null, 0x00);
	octet_uint8_write(db->row, device_row.metric_null, 0x00);
	octet_uint8_write(db->row, device_row.buffer_null, 0x00);

	if (octet_row_write(&stmt, file, offset, db->row, device_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	if (octet_mkdir(directory) == -1) {
		status = octet_error();
		goto cleanup;
	}

	const char *files[] = {uplink_file, downlink_file, reading_file, metric_file, buffer_file, config_file, radio_file};
	for (uint8_t index = 0; index < sizeof(files) / sizeof(files[0]); index++) {
		if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, files[index]) == -1) {
			error("failed to sprintf uuid to file\n");
			status = 500;
			goto cleanup;
		}

		if (octet_creat(file) == -1) {
			status = octet_error();
			goto cleanup;
		}
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t device_update(octet_t *db, device_t *device) {
	uint16_t status;

	if (device->zone_id != NULL) {
		char name[12];
		uint8_t color[12];
		zone_t zone = {.id = device->zone_id, .name = (char *)&name, .color = &color};
		status = zone_lookup(db, &zone);
		if (status != 0) {
			goto cleanup;
		}
		device->zone_id = zone.id;
		device->zone_name_len = zone.name_len;
		device->zone_name = zone.name;
		device->zone_color = zone.color;
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("update device %02x%02x updated at %lu\n", (*device->id)[0], (*device->id)[1], *device->updated_at);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("device %02x%02x not found\n", (*device->id)[0], (*device->id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, device_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, device_row.id);
		if (memcmp(id, device->id, sizeof(*device->id)) == 0) {
			if (device->name != NULL) {
				octet_uint8_write(db->row, device_row.name_len, device->name_len);
				octet_text_write(db->row, device_row.name, (char *)device->name, device->name_len);
			}
			if (device->zone_id != NULL) {
				octet_uint8_write(db->row, device_row.zone_null, 0x01);
				octet_blob_write(db->row, device_row.zone_id, (uint8_t *)device->zone_id, sizeof(*device->zone_id));
				octet_uint8_write(db->row, device_row.zone_name_len, device->zone_name_len);
				octet_text_write(db->row, device_row.zone_name, (char *)device->zone_name, device->zone_name_len);
				octet_blob_write(db->row, device_row.zone_color, (uint8_t *)device->zone_color, sizeof(*device->zone_color));
			}
			if (device->firmware != NULL) {
				octet_uint8_write(db->row, device_row.firmware_len, device->firmware_len);
				octet_text_write(db->row, device_row.firmware, device->firmware, device->firmware_len);
			}
			if (device->hardware != NULL) {
				octet_uint8_write(db->row, device_row.hardware_len, device->hardware_len);
				octet_text_write(db->row, device_row.hardware, device->hardware, device->hardware_len);
			}
			octet_uint8_write(db->row, device_row.updated_at_null, 0x01);
			octet_uint64_write(db->row, device_row.updated_at, (uint64_t)*device->updated_at);
			if (octet_row_write(&stmt, file, offset, db->row, device_row.size) == -1) {
				status = octet_error();
				goto cleanup;
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

void device_find_by_user(octet_t *db, request_t *request, response_t *response) {
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
	status = device_select_by_user(db, &user, &query, response, &devices_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu devices\n", devices_len);
	response->status = 200;
}

void device_modify(octet_t *db, request_t *request, response_t *response) {
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

	uint16_t status = device_update(db, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("updated device %02x%02x\n", (*device.id)[0], (*device.id)[1]);
	response->status = 200;
}
