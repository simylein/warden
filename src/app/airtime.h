#pragma once

#include "../api/uplink.h"
#include <stdint.h>

void airtime_account(uint8_t *airtime_ind, uint16_t (*airtime)[8], uplink_t *uplink);
uint16_t airtime_calculate(uint16_t (*airtime)[8]);
