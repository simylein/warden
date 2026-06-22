#pragma once

#include "../api/uplink.h"
#include <stdint.h>
#include <time.h>

void packet_account(uint8_t (*packet_rx)[8], uint8_t (*packet_lost)[8], time_t *packet_bucket, uint16_t *last_frame,
										uplink_t *uplink);
uint16_t packet_calculate(uint8_t (*packet_rx)[8], uint8_t (*packet_lost)[8], time_t packet_bucket);
