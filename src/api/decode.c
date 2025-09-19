#include "../lib/logger.h"
#include "buffer.h"
#include "metric.h"
#include "reading.h"
#include "uplink.h"
#include <stdint.h>
#include <time.h>

int decode_kind_00(uint8_t data_len, time_t received_at) {
	if (data_len != 0) {
		error("uplink data len must be 0 bytes\n");
		return -1;
	}

	trace("heartbeat received_at %lu\n", received_at);
	return 0;
}

int decode_kind_01(uint8_t *data, uint8_t data_len, time_t received_at, reading_t *reading) {
	if (data_len != 4) {
		error("uplink data len must be 4 bytes\n");
		return -1;
	}

	uint16_t temperature_raw = (uint16_t)(data[0] << 8) | (uint16_t)data[1];
	reading->temperature = ((175.72f * temperature_raw) / 65536.0f) - 46.85f;

	uint16_t humidity_raw = (uint16_t)(data[2] << 8) | (uint16_t)data[3];
	reading->humidity = ((125.0f * humidity_raw) / 65536.0f) - 6.0f;

	reading->captured_at = received_at;

	trace("temperature %.2f humidity %.2f captured_at %lu\n", reading->temperature, reading->humidity, reading->captured_at);
	return 0;
}

int decode_kind_02(uint8_t *data, uint8_t data_len, time_t received_at, metric_t *metric) {
	if (data_len != 3) {
		error("uplink data len must be 3 bytes\n");
		return -1;
	}

	uint16_t photovoltaic_raw = (uint16_t)(data[0] << 4) | (uint16_t)((data[1] >> 4) & 0x0f);
	metric->photovoltaic = (photovoltaic_raw * 3.3f) / 4095.0f * 2;

	uint16_t battery_raw = (uint16_t)((data[1] & 0x0f) << 8) | (uint16_t)data[2];
	metric->battery = (battery_raw * 3.3f) / 4095.0f * 2;

	metric->captured_at = received_at;

	trace("photovoltaic %.3f battery %.3f captured_at %lu\n", metric->photovoltaic, metric->battery, metric->captured_at);
	return 0;
}

int decode_kind_03(uint8_t *data, uint8_t data_len, time_t received_at, reading_t *reading, metric_t *metric) {
	if (data_len != 7) {
		error("uplink data len must be 7 bytes\n");
		return -1;
	}

	uint16_t temperature_raw = (uint16_t)(data[0] << 8) | (uint16_t)data[1];
	reading->temperature = ((175.72f * temperature_raw) / 65536.0f) - 46.85f;

	uint16_t humidity_raw = (uint16_t)(data[2] << 8) | (uint16_t)data[3];
	reading->humidity = ((125.0f * humidity_raw) / 65536.0f) - 6.0f;

	uint16_t photovoltaic_raw = (uint16_t)(data[4] << 4) | (uint16_t)((data[5] >> 4) & 0x0f);
	metric->photovoltaic = (photovoltaic_raw * 3.3f) / 4095.0f * 2;

	uint16_t battery_raw = (uint16_t)((data[5] & 0x0f) << 8) | (uint16_t)data[6];
	metric->battery = (battery_raw * 3.3f) / 4095.0f * 2;

	reading->captured_at = received_at;
	metric->captured_at = received_at;

	trace("temperature %.2f humidity %.2f captured_at %lu\n", reading->temperature, reading->humidity, reading->captured_at);
	trace("photovoltaic %.3f battery %.3f captured_at %lu\n", metric->photovoltaic, metric->battery, metric->captured_at);
	return 0;
}

int decode_kind_81(uint8_t *data, uint8_t data_len, time_t received_at, reading_t *reading, buffer_t *buffer) {
	if (data_len != 9) {
		error("uplink data len must be 9 bytes\n");
		return -1;
	}

	uint16_t temperature_raw = (uint16_t)(data[0] << 8) | (uint16_t)data[1];
	reading->temperature = ((175.72f * temperature_raw) / 65536.0f) - 46.85f;

	uint16_t humidity_raw = (uint16_t)(data[2] << 8) | (uint16_t)data[3];
	reading->humidity = ((125.0f * humidity_raw) / 65536.0f) - 6.0f;

	buffer->delay = (uint32_t)(data[4] << 16) | (uint32_t)(data[5] << 8) | (uint16_t)data[6];
	buffer->level = (uint16_t)(data[7] << 8) | (uint16_t)data[8];

	reading->captured_at = received_at - buffer->delay;
	buffer->captured_at = received_at;

	trace("temperature %.2f humidity %.2f captured_at %lu\n", reading->temperature, reading->humidity, reading->captured_at);
	trace("delay %u level %hu captured_at %lu\n", buffer->delay, buffer->level, buffer->captured_at);
	return 0;
}

int decode_kind_82(uint8_t *data, uint8_t data_len, time_t received_at, metric_t *metric, buffer_t *buffer) {
	if (data_len != 8) {
		error("uplink data len must be 8 bytes\n");
		return -1;
	}

	uint16_t photovoltaic_raw = (uint16_t)(data[0] << 4) | (uint16_t)((data[1] >> 4) & 0x0f);
	metric->photovoltaic = (photovoltaic_raw * 3.3f) / 4095.0f * 2;

	uint16_t battery_raw = (uint16_t)((data[1] & 0x0f) << 8) | (uint16_t)data[2];
	metric->battery = (battery_raw * 3.3f) / 4095.0f * 2;

	buffer->delay = (uint32_t)(data[3] << 16) | (uint32_t)(data[4] << 8) | (uint16_t)data[5];
	buffer->level = (uint16_t)(data[6] << 8) | (uint16_t)data[7];

	metric->captured_at = received_at - buffer->delay;
	buffer->captured_at = received_at;

	trace("photovoltaic %.3f battery %.3f captured_at %lu\n", metric->photovoltaic, metric->battery, metric->captured_at);
	trace("delay %u level %hu captured_at %lu\n", buffer->delay, buffer->level, buffer->captured_at);
	return 0;
}

int decode_kind_83(uint8_t *data, uint8_t data_len, time_t received_at, reading_t *reading, metric_t *metric,
									 buffer_t *buffer) {
	if (data_len != 12) {
		error("uplink data len must be 12 bytes\n");
		return -1;
	}

	uint16_t temperature_raw = (uint16_t)(data[0] << 8) | (uint16_t)data[1];
	reading->temperature = ((175.72f * temperature_raw) / 65536.0f) - 46.85f;

	uint16_t humidity_raw = (uint16_t)(data[2] << 8) | (uint16_t)data[3];
	reading->humidity = ((125.0f * humidity_raw) / 65536.0f) - 6.0f;

	uint16_t photovoltaic_raw = (uint16_t)(data[4] << 4) | (uint16_t)((data[5] >> 4) & 0x0f);
	metric->photovoltaic = (photovoltaic_raw * 3.3f) / 4095.0f * 2;

	uint16_t battery_raw = (uint16_t)((data[5] & 0x0f) << 8) | (uint16_t)data[6];
	metric->battery = (battery_raw * 3.3f) / 4095.0f * 2;

	buffer->delay = (uint32_t)(data[7] << 16) | (uint32_t)(data[8] << 8) | (uint16_t)data[9];
	buffer->level = (uint16_t)(data[10] << 8) | (uint16_t)data[11];

	reading->captured_at = received_at - buffer->delay;
	metric->captured_at = received_at - buffer->delay;
	buffer->captured_at = received_at;

	trace("temperature %.2f humidity %.2f captured_at %lu\n", reading->temperature, reading->humidity, reading->captured_at);
	trace("photovoltaic %.3f battery %.3f captured_at %lu\n", metric->photovoltaic, metric->battery, metric->captured_at);
	trace("delay %u level %hu captured_at %lu\n", buffer->delay, buffer->level, buffer->captured_at);
	return 0;
}

int decode(sqlite3 *database, uplink_t *uplink) {
	trace("decoding uplink kind %02x\n", uplink->kind);
	switch (uplink->kind) {
	case 0x00: {
		if (decode_kind_00(uplink->data_len, uplink->received_at) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		return 0;
	}
	case 0x01: {
		uint8_t id[16];
		reading_t reading = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		if (decode_kind_01(uplink->data, uplink->data_len, uplink->received_at, &reading) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		if (reading_insert(database, &reading) != 0) {
			return -1;
		}
		return 0;
	}
	case 0x02: {
		uint8_t id[16];
		metric_t metric = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		if (decode_kind_02(uplink->data, uplink->data_len, uplink->received_at, &metric) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		if (metric_insert(database, &metric) != 0) {
			return -1;
		}
		return 0;
	}
	case 0x03: {
		uint8_t id[16];
		reading_t reading = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		metric_t metric = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		if (decode_kind_03(uplink->data, uplink->data_len, uplink->received_at, &reading, &metric) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		if (reading_insert(database, &reading) != 0) {
			return -1;
		}
		if (metric_insert(database, &metric) != 0) {
			return -1;
		}
		return 0;
	}
	case 0x81: {
		uint8_t id[16];
		reading_t reading = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		buffer_t buffer = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		if (decode_kind_81(uplink->data, uplink->data_len, uplink->received_at, &reading, &buffer) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		if (reading_insert(database, &reading) != 0) {
			return -1;
		}
		if (buffer_insert(database, &buffer) != 0) {
			return -1;
		}
		return 0;
	}
	case 0x82: {
		uint8_t id[16];
		metric_t metric = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		buffer_t buffer = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		if (decode_kind_82(uplink->data, uplink->data_len, uplink->received_at, &metric, &buffer) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		if (metric_insert(database, &metric) != 0) {
			return -1;
		}
		if (buffer_insert(database, &buffer) != 0) {
			return -1;
		}
		return 0;
	}
	case 0x83: {
		uint8_t id[16];
		reading_t reading = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		metric_t metric = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		buffer_t buffer = {.id = &id, .uplink_id = uplink->id, .device_id = uplink->device_id};
		if (decode_kind_83(uplink->data, uplink->data_len, uplink->received_at, &reading, &metric, &buffer) == -1) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
			return -1;
		}
		if (reading_insert(database, &reading) != 0) {
			return -1;
		}
		if (metric_insert(database, &metric) != 0) {
			return -1;
		}
		if (buffer_insert(database, &buffer) != 0) {
			return -1;
		}
		return 0;
	}
	default: {
		error("unknown uplink kind %02x\n", uplink->kind);
		return -1;
	}
	}
}
