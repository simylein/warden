#pragma once

#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct config_t {
	uint8_t (*id)[16];
	bool led_debug;
	bool reading_enable;
	bool metric_enable;
	bool buffer_enable;
	uint16_t reading_interval;
	uint16_t metric_interval;
	uint16_t buffer_interval;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} config_t;

extern const char *config_table;
extern const char *config_schema;

uint16_t config_insert(sqlite3 *database, config_t *config);
