#include "../api/uplink.h"
#include <stdint.h>
#include <time.h>

void packet_account(uint8_t (*packet_rx)[8], uint8_t (*packet_lost)[8], time_t *packet_bucket, uint16_t *last_frame,
										uplink_t *uplink) {
	time_t bucket = (uplink->received_at / 900) * 900;
	if (bucket != *packet_bucket) {
		int32_t diff = (int32_t)((bucket - *packet_bucket) / 900);
		if (diff < 0) {
			return;
		}
		if (diff > 8) {
			diff = 8;
		}
		for (uint8_t index = 0; index < 8 - diff; index++) {
			(*packet_rx)[index] = (*packet_rx)[index + diff];
			(*packet_lost)[index] = (*packet_lost)[index + diff];
		}
		for (uint8_t index = 8 - (uint8_t)diff; index < 8; index++) {
			(*packet_rx)[index] = 0;
			(*packet_lost)[index] = 0;
		}
	}
	(*packet_rx)[7] = (*packet_rx)[7] + 1;
	if (*last_frame + 1 != uplink->frame) {
		(*packet_lost)[7] = (*packet_lost)[7] + (uint8_t)(uplink->frame - *last_frame - 1);
	}
	*packet_bucket = bucket;
}

uint16_t packet_calculate(uint8_t (*packet_rx)[8], uint8_t (*packet_lost)[8], time_t packet_bucket) {
	time_t age = time(NULL) - packet_bucket;
	if (age < 0) {
		age = 0;
	}
	uint8_t start_index = (uint8_t)(age / 900);
	if (start_index >= 8) {
		return 0;
	}
	uint16_t rx_sum = 0;
	for (uint8_t index = start_index; index < 8; index++) {
		rx_sum += (*packet_rx)[index];
	}
	uint16_t lost_sum = 0;
	for (uint8_t index = start_index; index < 8; index++) {
		lost_sum += (*packet_lost)[index];
	}
	uint16_t total = rx_sum + lost_sum;
	if (total == 0) {
		return 0;
	}
	return (uint16_t)((lost_sum * 10000u) / total);
}
