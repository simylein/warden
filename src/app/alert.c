#include "../api/alert.h"
#include "../api/buffer.h"
#include "../api/device.h"
#include "../api/metric.h"
#include "../api/reading.h"
#include "../api/rule.h"
#include "../lib/base16.h"
#include "../lib/config.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

pthread_t alerter_thread;
octet_t alerter_octet;
char *alerter_buffer;

int device_load(octet_t *db, uint8_t *devices_len) {
	int status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	if (stmt.stat.st_size > db->table_len) {
		error("file length %zu exceeds buffer length %u\n", (size_t)stmt.stat.st_size, db->table_len);
		status = -1;
		goto cleanup;
	}

	debug("select devices\n");

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->table[offset], device_row.size) == -1) {
			status = -1;
			goto cleanup;
		}
		*devices_len += 1;
		offset += device_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

int alert_load_by_device(octet_t *db, device_t *device, uint8_t *alerts_len) {
	int status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return -1;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, alert_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return -1;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	debug("select alerts for device %02x%02x\n", (*device->id)[0], (*device->id)[1]);

	time_t now = time(NULL);
	off_t offset = stmt.stat.st_size - alert_row.size;
	uint16_t alpha_len = 0;
	while (true) {
		if (offset < 0 || alpha_len + alert_row.size > db->alpha_len) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->alpha[alpha_len], alert_row.size) == -1) {
			status = -1;
			goto cleanup;
		}
		time_t issued_at = (time_t)octet_uint64_read(&db->alpha[alpha_len], alert_row.issued_at);
		if (issued_at + alert_lookback < now) {
			status = 0;
			break;
		}
		*alerts_len += 1;
		offset -= alert_row.size;
		alpha_len += alert_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

int rule_load_by_device(octet_t *db, device_t *device, uint8_t *rules_len) {
	int status;

	char uuid[16];
	if (base16_encode(uuid, sizeof(uuid), device->id, sizeof(*device->id)) == -1) {
		error("failed to encode uuid to base 16\n");
		return -1;
	}

	char file[128];
	if (sprintf(file, "%s/%.*s/%s.data", db->directory, (int)sizeof(uuid), uuid, rule_file) == -1) {
		error("failed to sprintf uuid to file\n");
		return -1;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	if (stmt.stat.st_size > db->bravo_len) {
		error("file length %zu exceeds buffer length %u\n", (size_t)stmt.stat.st_size, db->bravo_len);
		status = -1;
		goto cleanup;
	}

	debug("select rules for device %02x%02x\n", (*device->id)[0], (*device->id)[1]);

	off_t offset = 0;
	while (true) {
		if (offset >= stmt.stat.st_size) {
			status = 0;
			break;
		}
		if (octet_row_read(&stmt, file, offset, &db->bravo[offset], rule_row.size) == -1) {
			status = -1;
			goto cleanup;
		}
		*rules_len += 1;
		offset += rule_row.size;
	}

cleanup:
	octet_close(&stmt, file);
	return status;
}

int evaluate(rule_t *rule, alert_t *alert, float fraction, float value) {
	float activate = (float)rule->activate / fraction;
	float disable = (float)rule->disable / fraction;
	if (rule->edge == 1 && rule->activate > rule->disable) {
		if (alert == NULL && value >= activate) {
			return 1;
		} else if (alert != NULL && value <= disable) {
			return 0;
		} else {
			return alert != NULL ? 1 : 0;
		}
	} else if (rule->edge == 0 && rule->activate < rule->disable) {
		if (alert == NULL && value <= activate) {
			return 1;
		} else if (alert != NULL && value >= disable) {
			return 0;
		} else {
			return alert != NULL ? 1 : 0;
		}
	} else {
		warn("invalid rule edge %hhu activate %d disable %d on device %02x%02x\n", rule->edge, rule->activate, rule->disable,
				 (*rule->device_id)[0], (*rule->device_id)[1]);
		return -1;
	}
}

int alerting(rule_t *rule, device_t *device, int32_t *value, alert_t *alert) {
	switch (rule->field) {
	case 0:
		if (device->reading == NULL) {
			warn("device %02x%02x reading null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)(device->reading->temperature * 100);
		return evaluate(rule, alert, 100.0f, device->reading->temperature);
	case 1:
		if (device->reading == NULL) {
			warn("device %02x%02x reading null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)(device->reading->humidity * 100);
		return evaluate(rule, alert, 100.0f, device->reading->humidity);
	case 2:
		if (device->reading == NULL) {
			warn("device %02x%02x reading null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)(device->reading->dewpoint * 100);
		return evaluate(rule, alert, 100.0f, device->reading->dewpoint);
	case 3:
		if (device->metric == NULL) {
			warn("device %02x%02x metric null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)(device->metric->photovoltaic * 1000);
		return evaluate(rule, alert, 1000.0f, device->metric->photovoltaic);
	case 4:
		if (device->metric == NULL) {
			warn("device %02x%02x metric null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)(device->metric->battery * 1000);
		return evaluate(rule, alert, 1000.0f, device->metric->battery);
	case 5:
		if (device->buffer == NULL) {
			warn("device %02x%02x buffer null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)device->buffer->delay;
		return evaluate(rule, alert, 1.0f, (float)device->buffer->delay);
	case 6:
		if (device->buffer == NULL) {
			warn("device %02x%02x buffer null unable to evaluate rule\n", (*rule->device_id)[0], (*rule->device_id)[1]);
			return -1;
		}
		*value = (int32_t)device->buffer->level;
		return evaluate(rule, alert, 1.0f, (float)device->buffer->level);
	default:
		warn("invalid rule field %hhu\n", rule->field);
		return -1;
	}
}

void *alerter(void *args) {
	octet_t *db = (octet_t *)args;

	db->directory = database_directory;

	uint32_t offset = 0;

	db->row = (uint8_t *)&alerter_buffer[offset];
	db->row_len = UINT8_MAX;
	offset += db->row_len;

	db->alpha = (uint8_t *)&alerter_buffer[offset];
	db->alpha_len = (uint16_t)(database_buffer / 16);
	offset += db->alpha_len;

	db->bravo = (uint8_t *)&alerter_buffer[offset];
	db->bravo_len = (uint16_t)(database_buffer / 16);
	offset += db->bravo_len;

	db->table = (uint8_t *)&alerter_buffer[offset];
	db->table_len = database_buffer - offset;
	offset += db->table_len;

	while (true) {
		trace("alerter thread running thresholds\n");

		uint8_t devices_len = 0;
		if (device_load(db, &devices_len) == -1) {
			continue;
		}

		debug("found %hhu devices\n", devices_len);
		for (uint8_t index = 0; index < devices_len; index++) {
			reading_t reading;
			metric_t metric;
			buffer_t buffer;
			device_t device;
			device.id = (uint8_t (*)[8])octet_blob_read(&db->table[index * device_row.size], device_row.id);
			if (octet_uint8_read(&db->table[index * device_row.size], device_row.reading_null) != 0x00) {
				device.reading = &reading;
				int16_t temperature = octet_int16_read(&db->table[index * device_row.size], device_row.reading_temperature);
				uint16_t humidity = octet_uint16_read(&db->table[index * device_row.size], device_row.reading_humidity);
				int16_t dewpoint = octet_int16_read(&db->table[index * device_row.size], device_row.reading_dewpoint);
				device.reading->temperature = temperature / 100.0f;
				device.reading->humidity = humidity / 100.0f;
				device.reading->dewpoint = dewpoint / 100.0f;
			} else {
				device.reading = NULL;
			}
			if (octet_uint8_read(&db->table[index * device_row.size], device_row.metric_null) != 0x00) {
				device.metric = &metric;
				uint16_t photovoltaic = octet_uint16_read(&db->table[index * device_row.size], device_row.metric_photovoltaic);
				uint16_t battery = octet_uint16_read(&db->table[index * device_row.size], device_row.metric_battery);
				device.metric->photovoltaic = photovoltaic / 1000.0f;
				device.metric->battery = battery / 1000.0f;
			} else {
				device.metric = NULL;
			}
			if (octet_uint8_read(&db->table[index * device_row.size], device_row.buffer_null) != 0x00) {
				device.buffer = &buffer;
				device.buffer->delay = octet_uint32_read(&db->table[index * device_row.size], device_row.buffer_delay);
				device.buffer->level = octet_uint16_read(&db->table[index * device_row.size], device_row.buffer_level);
			} else {
				device.buffer = NULL;
			}

			uint8_t alerts_len = 0;
			if (alert_load_by_device(db, &device, &alerts_len) == -1) {
				continue;
			}

			debug("found %hhu alerts\n", alerts_len);

			uint8_t rules_len = 0;
			if (rule_load_by_device(db, &device, &rules_len) == -1) {
				continue;
			}

			debug("found %hhu rules\n", rules_len);
			for (uint8_t ind = 0; ind < rules_len; ind++) {
				rule_t rule = {.device_id = device.id};
				rule.severity = octet_uint8_read(&db->bravo[ind * rule_row.size], rule_row.severity);
				rule.field = octet_uint8_read(&db->bravo[ind * rule_row.size], rule_row.field);
				rule.edge = octet_uint8_read(&db->bravo[ind * rule_row.size], rule_row.edge);
				rule.activate = octet_int32_read(&db->bravo[ind * rule_row.size], rule_row.activate);
				rule.disable = octet_int32_read(&db->bravo[ind * rule_row.size], rule_row.disable);

				alert_t alert = {.device_id = device.id};
				alert_t *existing = NULL;
				for (uint8_t i = 0; i < alerts_len; i++) {
					alert.severity = octet_uint8_read(&db->alpha[i * alert_row.size], alert_row.severity);
					alert.field = octet_uint8_read(&db->alpha[i * alert_row.size], alert_row.field);
					alert.edge = octet_uint8_read(&db->alpha[i * alert_row.size], alert_row.edge);
					if (octet_uint8_read(&db->alpha[i * alert_row.size], alert_row.resolved_at_null) != 0x00) {
						alert.resolved_at = (time_t[]){(time_t)octet_uint64_read(&db->alpha[i * alert_row.size], alert_row.resolved_at)};
					} else {
						alert.resolved_at = NULL;
					}
					if (alert.severity == rule.severity && alert.field == rule.field && alert.edge == rule.edge &&
							alert.resolved_at == NULL) {
						existing = &alert;
						break;
					}
				}

				int32_t value;
				int result = alerting(&rule, &device, &value, existing);
				if (result == 1 && existing == NULL) {
					alert.severity = rule.severity;
					alert.field = rule.field;
					alert.edge = rule.edge;
					alert.value = value;
					alert.issued_at = time(NULL);
					alert.resolved_at = NULL;
					alert.device_id = device.id;
					if (alert_insert(db, &alert) != 0) {
						continue;
					}
					info("created alert issued at %lu\n", alert.issued_at);
				} else if (result == 0 && existing != NULL) {
					alert.severity = rule.severity;
					alert.field = rule.field;
					alert.edge = rule.edge;
					alert.device_id = device.id;
					alert.resolved_at = (time_t[]){time(NULL)};
					uint16_t status = alert_update(db, &alert);
					if (status == 0) {
						info("updated alert resolved at %lu\n", *alert.resolved_at);
					}
					if (status != 0) {
						continue;
					}
				}
			}
		}

		unsigned int duration = (uint8_t)(alert_interval - 4) + (uint8_t)(rand() % 9);
		trace("alerter thread sleeping for %hhus\n", duration);
		sleep(duration);
	}
}
