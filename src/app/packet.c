#include "../api/uplink.h"
#include <stdint.h>
#include <time.h>

void packet_account(uint8_t (*packet_rx)[32], uint8_t (*packet_lost)[32], time_t *packet_bucket, uint16_t *last_frame,
										uplink_t *uplink) {
	time_t bucket = (uplink->received_at / 900) * 900;
	if (bucket != *packet_bucket) {
		int32_t diff = (int32_t)((bucket - *packet_bucket) / 900);
		if (diff < 0) {
			return;
		}
		if (diff > (uint8_t)sizeof(*packet_rx)) {
			diff = sizeof(*packet_rx);
		}
		for (uint8_t index = 0; index < (uint8_t)sizeof(*packet_rx) - diff; index++) {
			(*packet_rx)[index] = (*packet_rx)[index + diff];
			(*packet_lost)[index] = (*packet_lost)[index + diff];
		}
		for (uint8_t index = sizeof(*packet_rx) - (uint8_t)diff; index < sizeof(*packet_rx); index++) {
			(*packet_rx)[index] = 0;
			(*packet_lost)[index] = 0;
		}
	}
	(*packet_rx)[sizeof(*packet_rx) - 1] = (*packet_rx)[sizeof(*packet_rx) - 1] + 1;
	if (*last_frame + 1 != uplink->frame) {
		(*packet_lost)[sizeof(*packet_rx) - 1] =
				(*packet_lost)[sizeof(*packet_rx) - 1] + (uint8_t)(uplink->frame - *last_frame - 1);
	}
	*packet_bucket = bucket;
}

uint16_t packet_calculate(uint8_t (*packet_rx)[32], uint8_t (*packet_lost)[32], time_t packet_bucket) {
	time_t age = time(NULL) - packet_bucket;
	if (age < 0) {
		age = 0;
	}
	uint8_t start_index = (uint8_t)(age / 900);
	if (start_index >= sizeof(*packet_rx)) {
		return 0;
	}
	uint32_t rx_sum = 0;
	uint32_t lost_sum = 0;
	for (uint8_t index = start_index; index < sizeof(*packet_rx); index++) {
		uint16_t weight = (index == sizeof(*packet_rx) - 1) ? (uint16_t)(age % 900) : 900;
		rx_sum += (*packet_rx)[index] * weight;
		lost_sum += (*packet_lost)[index] * weight;
	}
	uint32_t total = rx_sum + lost_sum;
	if (total == 0) {
		return 0;
	}
	return (uint16_t)((lost_sum * 10000u) / total);
}
