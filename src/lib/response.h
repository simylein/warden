#pragma once

#include <stdint.h>
#include <stdio.h>

typedef struct request_t request_t;

typedef struct response_t {
	uint16_t status;
	char (*head)[128];
	uint8_t head_len;
	char (*header)[2048];
	uint16_t header_len;
	char (*body)[63360];
	size_t body_len;
} response_t;

void response_init(response_t *response, char *buffer);
size_t response(char *buffer, request_t *req, response_t *res);

void append_header(response_t *response, const char *format, ...) __attribute__((format(printf, 2, 3)));
void append_body(response_t *response, const void *buffer, size_t buffer_len);
