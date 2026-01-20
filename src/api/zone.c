#include "zone.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "cache.h"
#include "user-zone.h"
#include "user.h"
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char *zone_table = "zone";
const char *zone_schema = "create table zone ("
													"id blob primary key, "
													"name text not null unique, "
													"color blob not null, "
													"created_at timestamp not null, "
													"updated_at timestamp"
													")";

const char *zone_file = "zone";

const zone_row_t zone_row = {
		.id = 0,
		.name_len = 16,
		.name = 17,
		.color = 29,
		.created_at = 41,
		.updated_at_null = 49,
		.updated_at = 50,
		.reading_null = 58,
		.reading_temperature = 59,
		.reading_humidity = 61,
		.reading_captured_at = 63,
		.metric_null = 71,
		.metric_photovoltaic = 72,
		.metric_battery = 74,
		.metric_captured_at = 76,
		.buffer_null = 84,
		.buffer_delay = 85,
		.buffer_level = 89,
		.buffer_captured_at = 91,
		.size = 93,
};

int zone_rowcmp(uint8_t *alpha, uint8_t *bravo, zone_query_t *query) {
	if (query->order_len == 2 && memcmp(query->order, "id", query->order_len) == 0) {
		uint64_t id_alpha = octet_uint64_read(alpha, zone_row.id);
		uint64_t id_bravo = octet_uint64_read(bravo, zone_row.id);
		int result = (id_alpha > id_bravo) - (id_alpha < id_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 4 && memcmp(query->order, "name", query->order_len) == 0) {
		uint8_t name_len_alpha = octet_uint8_read(alpha, zone_row.name_len);
		char *name_alpha = octet_text_read(alpha, zone_row.name);
		uint8_t name_len_bravo = octet_uint8_read(bravo, zone_row.name_len);
		char *name_bravo = octet_text_read(bravo, zone_row.name);
		int result = memcmp(name_alpha, name_bravo, name_len_alpha < name_len_bravo ? name_len_alpha : name_len_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 9 && memcmp(query->order, "createdAt", query->order_len) == 0) {
		time_t created_at_alpha = (time_t)octet_uint64_read(alpha, zone_row.created_at);
		time_t created_at_bravo = (time_t)octet_uint64_read(bravo, zone_row.created_at);
		int result = (created_at_alpha > created_at_bravo) - (created_at_alpha < created_at_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 9 && memcmp(query->order, "updatedAt", query->order_len) == 0) {
		time_t updated_at_alpha = (time_t)octet_uint64_read(alpha, zone_row.updated_at);
		time_t updated_at_bravo = (time_t)octet_uint64_read(bravo, zone_row.updated_at);
		int result = (updated_at_alpha > updated_at_bravo) - (updated_at_alpha < updated_at_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 11 && memcmp(query->order, "temperature", query->order_len) == 0) {
		int16_t temperature_alpha = octet_int16_read(alpha, zone_row.reading_temperature);
		int16_t temperature_bravo = octet_int16_read(bravo, zone_row.reading_temperature);
		int result = (temperature_alpha > temperature_bravo) - (temperature_alpha < temperature_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 8 && memcmp(query->order, "humidity", query->order_len) == 0) {
		uint16_t humidity_alpha = octet_uint16_read(alpha, zone_row.reading_humidity);
		uint16_t humidity_bravo = octet_uint16_read(bravo, zone_row.reading_humidity);
		int result = (humidity_alpha > humidity_bravo) - (humidity_alpha < humidity_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 12 && memcmp(query->order, "photovoltaic", query->order_len) == 0) {
		uint16_t photovoltaic_alpha = octet_uint16_read(alpha, zone_row.metric_photovoltaic);
		uint16_t photovoltaic_bravo = octet_uint16_read(bravo, zone_row.metric_photovoltaic);
		int result = (photovoltaic_alpha > photovoltaic_bravo) - (photovoltaic_alpha < photovoltaic_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 7 && memcmp(query->order, "battery", query->order_len) == 0) {
		uint16_t battery_alpha = octet_uint16_read(alpha, zone_row.metric_battery);
		uint16_t battery_bravo = octet_uint16_read(bravo, zone_row.metric_battery);
		int result = (battery_alpha > battery_bravo) - (battery_alpha < battery_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 5 && memcmp(query->order, "delay", query->order_len) == 0) {
		uint32_t delay_alpha = octet_uint32_read(alpha, zone_row.buffer_delay);
		uint32_t delay_bravo = octet_uint32_read(bravo, zone_row.buffer_delay);
		int result = (delay_alpha > delay_bravo) - (delay_alpha < delay_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	if (query->order_len == 5 && memcmp(query->order, "level", query->order_len) == 0) {
		uint16_t level_alpha = octet_uint16_read(alpha, zone_row.buffer_level);
		uint16_t level_bravo = octet_uint16_read(bravo, zone_row.buffer_level);
		int result = (level_alpha > level_bravo) - (level_alpha < level_bravo);
		if (query->sort_len == 4 && memcmp(query->sort, "desc", query->sort_len) == 0) {
			result = -result;
		}
		return result;
	}

	return 0;
}

uint16_t zone_existing(octet_t *db, zone_t *zone) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select existing zone %02x%02x\n", (*zone->id)[0], (*zone->id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("zone %02x%02x not found\n", (*zone->id)[0], (*zone->id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, zone_row.id);
		if (memcmp(id, zone->id, sizeof(*zone->id)) == 0) {
			status = 0;
			break;
		}
		offset += zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t zone_lookup(octet_t *db, zone_t *zone) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select existing zone %02x%02x\n", (*zone->id)[0], (*zone->id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("zone %02x%02x not found\n", (*zone->id)[0], (*zone->id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, zone_row.id);
		if (memcmp(id, zone->id, sizeof(*zone->id)) == 0) {
			uint8_t name_len = octet_uint8_read(db->row, zone_row.name_len);
			char *name = octet_text_read(db->row, zone_row.name);
			uint8_t (*color)[12] = (uint8_t (*)[12])octet_blob_read(db->row, zone_row.color);
			zone->name_len = name_len;
			memcpy(zone->name, name, name_len);
			memcpy(zone->color, color, sizeof(*color));
			status = 0;
			break;
		}
		offset += zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t zone_select(octet_t *db, bwt_t *bwt, zone_query_t *query, response_t *response, uint8_t *zones_len) {
	uint16_t status;

	uint8_t user_zones_len = 0;
	user_t user = {.id = &bwt->id};
	status = user_zone_select_by_user(db, &user, &user_zones_len);
	if (status != 0) {
		return status;
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
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

	debug("select zones for user %02x%02x order by %.*s:%.*s\n", bwt->id[0], bwt->id[1], (int)query->order_len, query->order,
				(int)query->sort_len, query->sort);

	off_t offset = 0;
	uint32_t table_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->table[table_len], zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[table_len], zone_row.id);
		for (uint8_t index = 0; index < user_zones_len; index++) {
			uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_zone_row.size], user_zone_row.zone_id);
			if (memcmp(id, zone_id, sizeof(*zone_id)) == 0) {
				table_len += zone_row.size;
				break;
			}
		}
		offset += zone_row.size;
	}

	if (table_len >= zone_row.size * 2) {
		for (uint8_t index = 0; index < table_len / zone_row.size - 1; index++) {
			for (uint8_t ind = index + 1; ind < table_len / zone_row.size; ind++) {
				if (zone_rowcmp(&db->table[index * zone_row.size], &db->table[ind * zone_row.size], query) > 0) {
					memcpy(db->row, &db->table[index * zone_row.size], zone_row.size);
					memcpy(&db->table[index * zone_row.size], &db->table[ind * zone_row.size], zone_row.size);
					memcpy(&db->table[ind * zone_row.size], db->row, zone_row.size);
				}
			}
		}
	}

	uint32_t index = zone_row.size * query->offset;
	while (true) {
		if (index >= table_len || *zones_len >= query->limit) {
			status = 0;
			break;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], zone_row.id);
		uint8_t name_len = octet_uint8_read(&db->table[index], zone_row.name_len);
		char *name = octet_text_read(&db->table[index], zone_row.name);
		uint8_t (*color)[12] = (uint8_t (*)[12])octet_blob_read(&db->table[index], zone_row.color);
		uint8_t reading_null = octet_uint8_read(&db->table[index], zone_row.reading_null);
		int16_t reading_temperature = octet_int16_read(&db->table[index], zone_row.reading_temperature);
		uint16_t reading_humidity = octet_uint16_read(&db->table[index], zone_row.reading_humidity);
		time_t reading_captured_at = (time_t)octet_uint64_read(&db->table[index], zone_row.reading_captured_at);
		uint8_t metric_null = octet_uint8_read(&db->table[index], zone_row.metric_null);
		uint16_t metric_photovoltaic = octet_uint16_read(&db->table[index], zone_row.metric_photovoltaic);
		uint16_t metric_battery = octet_uint16_read(&db->table[index], zone_row.metric_battery);
		time_t metric_captured_at = (time_t)octet_uint64_read(&db->table[index], zone_row.metric_captured_at);
		uint8_t buffer_null = octet_uint8_read(&db->table[index], zone_row.buffer_null);
		uint32_t buffer_delay = octet_uint32_read(&db->table[index], zone_row.buffer_delay);
		uint16_t buffer_level = octet_uint16_read(&db->table[index], zone_row.buffer_level);
		time_t buffer_captured_at = (time_t)octet_uint64_read(&db->table[index], zone_row.buffer_captured_at);
		body_write(response, id, sizeof(*id));
		body_write(response, name, name_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, color, sizeof(*color));
		body_write(response, (uint8_t[]){reading_null != 0x00}, sizeof(reading_null));
		if (reading_null != 0x00) {
			body_write(response, (uint16_t[]){hton16((uint16_t)reading_temperature)}, sizeof(reading_temperature));
			body_write(response, (uint16_t[]){hton16(reading_humidity)}, sizeof(reading_humidity));
			body_write(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
		}
		body_write(response, (uint8_t[]){metric_null != 0x00}, sizeof(metric_null));
		if (metric_null != 0x00) {
			body_write(response, (uint16_t[]){hton16(metric_photovoltaic)}, sizeof(metric_photovoltaic));
			body_write(response, (uint16_t[]){hton16(metric_battery)}, sizeof(metric_battery));
			body_write(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
		}
		body_write(response, (uint8_t[]){buffer_null != 0x00}, sizeof(buffer_null));
		if (buffer_null != 0x00) {
			body_write(response, (uint32_t[]){hton32(buffer_delay)}, sizeof(buffer_delay));
			body_write(response, (uint16_t[]){hton16(buffer_level)}, sizeof(buffer_level));
			body_write(response, (uint64_t[]){hton64((uint64_t)buffer_captured_at)}, sizeof(buffer_captured_at));
		}
		cache_zone_t cache_zone;
		memcpy(cache_zone.id, id, sizeof(cache_zone.id));
		memcpy(cache_zone.name, name, name_len);
		cache_zone.name_len = (uint8_t)name_len;
		if (cache_zone_write(&cache_zone) == -1) {
			warn("failed to cache zone %02x%02x\n", (*id)[0], (*id)[1]);
		}
		*zones_len += 1;
		index += zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t zone_select_one(octet_t *db, bwt_t *bwt, zone_t *zone, response_t *response) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select zone %02x%02x for user %02x%02x\n", (*zone->id)[0], (*zone->id)[1], bwt->id[0], bwt->id[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, zone_row.id);
		if (memcmp(id, zone->id, sizeof(*zone->id)) == 0) {
			uint8_t name_len = octet_uint8_read(db->row, zone_row.name_len);
			char *name = octet_text_read(db->row, zone_row.name);
			uint8_t (*color)[12] = (uint8_t (*)[12])octet_blob_read(db->row, zone_row.color);
			time_t created_at = (time_t)octet_uint64_read(db->row, zone_row.created_at);
			uint8_t updated_at_null = octet_uint8_read(db->row, zone_row.updated_at_null);
			time_t updated_at = (time_t)octet_uint64_read(db->row, zone_row.updated_at);
			uint8_t reading_null = octet_uint8_read(db->row, zone_row.reading_null);
			int16_t reading_temperature = octet_int16_read(db->row, zone_row.reading_temperature);
			uint16_t reading_humidity = octet_uint16_read(db->row, zone_row.reading_humidity);
			time_t reading_captured_at = (time_t)octet_uint64_read(db->row, zone_row.reading_captured_at);
			uint8_t metric_null = octet_uint8_read(db->row, zone_row.metric_null);
			uint16_t metric_photovoltaic = octet_uint16_read(db->row, zone_row.metric_photovoltaic);
			uint16_t metric_battery = octet_uint16_read(db->row, zone_row.metric_battery);
			time_t metric_captured_at = (time_t)octet_uint64_read(db->row, zone_row.metric_captured_at);
			uint8_t buffer_null = octet_uint8_read(db->row, zone_row.buffer_null);
			uint32_t buffer_delay = octet_uint32_read(db->row, zone_row.buffer_delay);
			uint16_t buffer_level = octet_uint16_read(db->row, zone_row.buffer_level);
			time_t buffer_captured_at = (time_t)octet_uint64_read(db->row, zone_row.buffer_captured_at);
			body_write(response, id, sizeof(*id));
			body_write(response, name, name_len);
			body_write(response, (char[]){0x00}, sizeof(char));
			body_write(response, color, sizeof(*color));
			body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
			body_write(response, (uint8_t[]){updated_at_null != 0x00}, sizeof(updated_at_null));
			if (updated_at_null != 0x00) {
				body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
			}
			body_write(response, (uint8_t[]){reading_null != 0x00}, sizeof(reading_null));
			if (reading_null != 0x00) {
				body_write(response, (uint16_t[]){hton16((uint16_t)reading_temperature)}, sizeof(reading_temperature));
				body_write(response, (uint16_t[]){hton16(reading_humidity)}, sizeof(reading_humidity));
				body_write(response, (uint64_t[]){hton64((uint64_t)reading_captured_at)}, sizeof(reading_captured_at));
			}
			body_write(response, (uint8_t[]){metric_null != 0x00}, sizeof(metric_null));
			if (metric_null != 0x00) {
				body_write(response, (uint16_t[]){hton16(metric_photovoltaic)}, sizeof(metric_photovoltaic));
				body_write(response, (uint16_t[]){hton16(metric_battery)}, sizeof(metric_battery));
				body_write(response, (uint64_t[]){hton64((uint64_t)metric_captured_at)}, sizeof(metric_captured_at));
			}
			body_write(response, (uint8_t[]){buffer_null != 0x00}, sizeof(buffer_null));
			if (buffer_null != 0x00) {
				body_write(response, (uint32_t[]){hton32(buffer_delay)}, sizeof(buffer_delay));
				body_write(response, (uint16_t[]){hton16(buffer_level)}, sizeof(buffer_level));
				body_write(response, (uint64_t[]){hton64((uint64_t)buffer_captured_at)}, sizeof(buffer_captured_at));
			}
			cache_zone_t cache_zone;
			memcpy(cache_zone.id, id, sizeof(cache_zone.id));
			memcpy(cache_zone.name, name, name_len);
			cache_zone.name_len = (uint8_t)name_len;
			if (cache_zone_write(&cache_zone) == -1) {
				warn("failed to cache zone %02x%02x\n", (*zone->id)[0], (*zone->id)[1]);
			}
			status = 0;
			break;
		}
		offset += zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t zone_select_by_user(octet_t *db, user_t *user, zone_query_t *query, response_t *response, uint8_t *zones_len) {
	uint16_t status;

	uint8_t user_zones_len = 0;
	status = user_zone_select_by_user(db, user, &user_zones_len);
	if (status != 0) {
		return status;
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
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

	debug("select zones for user %02x%02x order by %.*s:%.*s\n", (*user->id)[0], (*user->id)[1], (int)query->order_len,
				query->order, (int)query->sort_len, query->sort);

	off_t offset = 0;
	uint32_t table_len = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->table[table_len], zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[table_len], zone_row.id);
		for (uint8_t index = 0; index < user_zones_len; index++) {
			uint8_t (*zone_id)[16] = (uint8_t (*)[16])octet_blob_read(&db->chunk[index * user_zone_row.size], user_zone_row.zone_id);
			if (memcmp(id, zone_id, sizeof(*zone_id)) == 0) {
				table_len += zone_row.size;
				break;
			}
		}
		offset += zone_row.size;
	}

	if (table_len >= zone_row.size * 2) {
		for (uint8_t index = 0; index < table_len / zone_row.size - 1; index++) {
			for (uint8_t ind = index + 1; ind < table_len / zone_row.size; ind++) {
				if (zone_rowcmp(&db->table[index * zone_row.size], &db->table[ind * zone_row.size], query) > 0) {
					memcpy(db->row, &db->table[index * zone_row.size], zone_row.size);
					memcpy(&db->table[index * zone_row.size], &db->table[ind * zone_row.size], zone_row.size);
					memcpy(&db->table[ind * zone_row.size], db->row, zone_row.size);
				}
			}
		}
	}

	uint32_t index = zone_row.size * query->offset;
	while (true) {
		if (index >= table_len || *zones_len >= query->limit) {
			status = 0;
			break;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(&db->table[index], zone_row.id);
		uint8_t name_len = octet_uint8_read(&db->table[index], zone_row.name_len);
		char *name = octet_text_read(&db->table[index], zone_row.name);
		uint8_t (*color)[12] = (uint8_t (*)[12])octet_blob_read(&db->table[index], zone_row.color);
		time_t created_at = (time_t)octet_uint64_read(&db->table[index], zone_row.created_at);
		uint8_t updated_at_null = octet_uint8_read(&db->table[index], zone_row.updated_at_null);
		time_t updated_at = (time_t)octet_uint64_read(&db->table[index], zone_row.updated_at);
		uint8_t uplink_null = 0x00;
		body_write(response, id, sizeof(*id));
		body_write(response, name, name_len);
		body_write(response, (char[]){0x00}, sizeof(char));
		body_write(response, color, sizeof(*color));
		body_write(response, (uint64_t[]){hton64((uint64_t)created_at)}, sizeof(created_at));
		body_write(response, (uint8_t[]){updated_at_null != 0x00}, sizeof(updated_at_null));
		if (updated_at_null != 0x00) {
			body_write(response, (uint64_t[]){hton64((uint64_t)updated_at)}, sizeof(updated_at));
		}
		body_write(response, (uint8_t[]){uplink_null != 0x00}, sizeof(uplink_null));
		*zones_len += 1;
		index += zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

int zone_parse(zone_t *zone, request_t *request) {
	request->body.pos = 0;

	uint8_t stage = 0;

	zone->name_len = 0;
	const uint8_t name_index = (uint8_t)request->body.pos;
	while (stage == 0 && zone->name_len < 12 && request->body.pos < request->body.len) {
		const char *byte = body_read(request, sizeof(char));
		if (*byte == '\0') {
			stage = 1;
		} else {
			zone->name_len++;
		}
	}
	if (zone->name_len != 0) {
		zone->name = &request->body.ptr[name_index];
	} else {
		zone->name = NULL;
	}
	if (stage != 1) {
		debug("found name with %hhu bytes\n", zone->name_len);
		return -1;
	}

	if (request->body.len < request->body.pos + sizeof(*zone->color)) {
		debug("missing color on zone\n");
		return -1;
	}
	zone->color = (uint8_t (*)[12])body_read(request, sizeof(*zone->color));
	stage = 2;

	return 0;
}

int zone_validate(zone_t *zone) {
	if (zone->name_len < 2) {
		return -1;
	}

	uint8_t name_index = 0;
	while (name_index < zone->name_len) {
		char *byte = &zone->name[name_index];
		if ((*byte < 'a' || *byte > 'z') && (*byte < '0' || *byte > '9')) {
			debug("name contains invalid character %02x\n", *byte);
			return -1;
		}
		name_index++;
	}

	return 0;
}

uint16_t zone_insert(octet_t *db, zone_t *zone) {
	uint16_t status;

	for (uint8_t index = 0; index < sizeof(*zone->id); index++) {
		(*zone->id)[index] = (uint8_t)(rand() & 0xff);
	}

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert zone name %.*s created at %lu\n", zone->name_len, zone->name, *zone->created_at);

	octet_blob_write(db->row, zone_row.id, (uint8_t *)zone->id, sizeof(*zone->id));
	octet_uint8_write(db->row, zone_row.name_len, zone->name_len);
	octet_text_write(db->row, zone_row.name, (char *)zone->name, zone->name_len);
	octet_blob_write(db->row, zone_row.color, (uint8_t *)zone->color, sizeof(*zone->color));
	octet_uint64_write(db->row, zone_row.created_at, (uint64_t)*zone->created_at);
	octet_uint8_write(db->row, zone_row.updated_at_null, 0x00);
	octet_uint8_write(db->row, zone_row.reading_null, 0x00);
	octet_uint8_write(db->row, zone_row.metric_null, 0x00);
	octet_uint8_write(db->row, zone_row.buffer_null, 0x00);

	off_t offset = stmt.stat.st_size;
	if (octet_row_write(&stmt, file, offset, db->row, zone_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t zone_update(octet_t *db, zone_t *zone) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("update zone %02x%02x name %.*s\n", (*zone->id)[0], (*zone->id)[1], zone->name_len, zone->name);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			warn("zone %02x%02x not found\n", (*zone->id)[0], (*zone->id)[1]);
			status = 404;
			break;
		}
		if (octet_row_read(&stmt, file, offset, db->row, zone_row.size) == -1) {
			status = octet_error();
			goto cleanup;
		}
		uint8_t (*id)[16] = (uint8_t (*)[16])octet_blob_read(db->row, zone_row.id);
		if (memcmp(id, zone->id, sizeof(*zone->id)) == 0) {
			octet_uint8_write(db->row, zone_row.name_len, zone->name_len);
			octet_text_write(db->row, zone_row.name, (char *)zone->name, zone->name_len);
			octet_blob_write(db->row, zone_row.color, (uint8_t *)zone->color, sizeof(*zone->color));
			octet_uint8_write(db->row, zone_row.updated_at_null, 0x01);
			octet_uint64_write(db->row, zone_row.updated_at, (uint64_t)*zone->updated_at);
			if (octet_row_write(&stmt, file, offset, db->row, zone_row.size) == -1) {
				status = octet_error();
				goto cleanup;
			}
			status = 0;
			break;
		}
		offset += zone_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

void zone_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	zone_query_t query = {.limit = 16, .offset = 0};
	if (strnfind(request->search.ptr, request->search.len, "order=", "&", &query.order, &query.order_len, 16) == -1) {
		response->status = 400;
		return;
	}

	if (strnfind(request->search.ptr, request->search.len, "sort=", "", &query.sort, &query.sort_len, 8) == -1) {
		response->status = 400;
		return;
	}

	uint8_t zones_len = 0;
	uint16_t status = zone_select(db, bwt, &query, response, &zones_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu zones\n", zones_len);
	response->status = 200;
}

void zone_find_one(octet_t *db, bwt_t *bwt, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

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

	zone_t zone = {.id = &id};
	uint16_t status = zone_existing(db, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	status = zone_select_one(db, bwt, &zone, response);
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
	info("found zone %02x%02x\n", (*zone.id)[0], (*zone.id)[1]);
	response->status = 200;
}

void zone_find_by_user(octet_t *db, request_t *request, response_t *response) {
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

	zone_query_t query = {.limit = 16, .offset = 0};
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

	uint8_t zones_len = 0;
	status = zone_select_by_user(db, &user, &query, response, &zones_len);
	if (status != 0) {
		response->status = status;
		return;
	}

	header_write(response, "content-type:application/octet-stream\r\n");
	header_write(response, "content-length:%u\r\n", response->body.len);
	info("found %hhu zones\n", zones_len);
	response->status = 200;
}

void zone_modify(octet_t *db, request_t *request, response_t *response) {
	if (request->search.len != 0) {
		response->status = 400;
		return;
	}

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

	time_t now = time(NULL);
	zone_t zone = {.id = &id, .updated_at = &now};
	if (request->body.len == 0 || zone_parse(&zone, request) == -1 || zone_validate(&zone) == -1) {
		response->status = 400;
		return;
	}

	uint16_t status = zone_update(db, &zone);
	if (status != 0) {
		response->status = status;
		return;
	}

	info("updated zone %02x%02x\n", (*zone.id)[0], (*zone.id)[1]);
	response->status = 200;
}
