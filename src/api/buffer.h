#pragma once

#include <stdint.h>
#include <time.h>

typedef struct buffer_t {
	uint8_t (*id)[16];
	uint32_t delay;
	uint16_t level;
	time_t captured_at;
	uint8_t (*uplink_id)[16];
	uint8_t (*device_id)[16];
} buffer_t;
