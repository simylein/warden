#include "airtime.h"
#include "../api/uplink.h"
#include "../lib/endian.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

void airtime_account(uint8_t *airtime_ind, uint16_t (*airtime)[8], uplink_t *uplink) {
	uint8_t bucket = (uint8_t)((uint64_t)(uplink->received_at / 900) % (sizeof(*airtime) / sizeof((*airtime)[0])));
	if (bucket != *airtime_ind) {
		uint8_t index = (*airtime_ind + 1) % (sizeof(*airtime) / sizeof((*airtime)[0]));
		while (index != bucket) {
			(*airtime)[index] = 0;
			index = (index + 1) % (sizeof(*airtime) / sizeof((*airtime)[0]));
		}
	}
	uint16_t time = ntoh16((*airtime)[bucket]) + uplink->airtime / 16;
	memcpy(&(*airtime)[bucket], (uint16_t[]){hton16(time)}, sizeof((*airtime)[0]));
	*airtime_ind = bucket;
}

uint16_t airtime_calculate(uint16_t (*airtime)[8]) {
	uint32_t sum = 0;
	uint16_t span = (uint16_t)(7 * 900 + time(NULL) % 900);
	for (uint8_t index = 0; index < sizeof(*airtime) / sizeof((*airtime)[0]); index++) {
		sum += ntoh16((*airtime)[index]);
	}
	return (uint16_t)(sum / (span / 100));
}
