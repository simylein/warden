#pragma once

#include "../api/uplink.h"
#include <stdint.h>

void airtime_account(uint16_t (*airtime)[8], time_t *airtime_bucket, uplink_t *uplink);
uint16_t airtime_calculate(uint16_t (*airtime)[8], time_t airtime_bucket);
