#pragma once

#include <stdint.h>
#include <time.h>

typedef struct bwt_t {
	uint8_t id[16];
	time_t iat;
	time_t exp;
	uint8_t data[8];
} bwt_t;

int bwt_sign(char (*buffer)[116], uint8_t (*id)[16], uint8_t (*data)[8]);
int bwt_verify(const char *cookie, const size_t cookie_len, bwt_t *bwt);
