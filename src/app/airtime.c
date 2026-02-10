#include "airtime.h"
#include "../api/uplink.h"
#include "../lib/endian.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void airtime_account(uint16_t (*airtime)[8], time_t *airtime_bucket, uplink_t *uplink) {
	time_t bucket = (uplink->received_at / 900) * 900;
	if (bucket != *airtime_bucket) {
		int32_t diff = (int32_t)((bucket - *airtime_bucket) / 900);
		if (diff < 0) {
			return;
		}
		if (diff > 8) {
			diff = 8;
		}
		for (uint8_t index = 0; index < 8 - diff; index++) {
			(*airtime)[index] = (*airtime)[index + diff];
		}
		for (uint8_t index = 8 - (uint8_t)diff; index < 8; index++) {
			(*airtime)[index] = 0;
		}
	}
	uint16_t time = ntoh16((*airtime)[7]) + uplink->airtime / 16;
	(*airtime)[7] = hton16(time);
	*airtime_bucket = bucket;
}

uint16_t airtime_calculate(uint16_t (*airtime)[8], time_t airtime_bucket) {
	time_t age = time(NULL) - airtime_bucket;
	if (age < 0) {
		age = 0;
	}
	uint8_t start_index = (uint8_t)(age / 900);
	if (start_index >= 8) {
		return 0;
	}
	uint32_t sum = 0;
	for (uint8_t index = start_index; index < 8; index++) {
		sum += ntoh16((*airtime)[index]);
	}
	uint32_t span = (uint32_t)((7 - start_index) * 900 + age % 900) * 1000;
	if (span == 0) {
		return 0;
	}
	return (uint16_t)(((float)sum / (float)span) * 100.0f * 1000.0f);
}
