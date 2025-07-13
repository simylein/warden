#pragma once

#include "response.h"
#include <stdint.h>

typedef struct request_t {
	char (*method)[8];
	uint8_t method_len;
	char (*pathname)[128];
	uint8_t pathname_len;
	char (*search)[256];
	uint16_t search_len;
	char (*protocol)[16];
	uint8_t protocol_len;
	char (*header)[2048];
	uint16_t header_len;
	char (*body)[128616];
	size_t body_len;
} request_t;

void request_init(request_t *request);
void request(char *buffer, size_t length, request_t *req, response_t *res);

const char *find_param(request_t *request, uint8_t offset, uint8_t *length);
const char *find_header(request_t *request, const char *key);
