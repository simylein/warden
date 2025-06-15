#include "../lib/logger.h"
#include "reading.h"
#include "uplink.h"
#include <stdint.h>
#include <time.h>

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

int decode(uplink_t *uplink) {
	trace("decoding uplink kind %02x\n", uplink->kind);
	switch (uplink->kind) {
	case 0x01: {
		reading_t reading;
		if (decode_kind_01(uplink->data, uplink->data_len, uplink->received_at, &reading)) {
			error("failed to decode uplink kind %02x\n", uplink->kind);
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
