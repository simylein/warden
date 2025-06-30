#include "response.h"
#include "request.h"
#include "status.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void response_init(response_t *response, char *buffer) {
	response->status = 0;
	response->head = (char (*)[128])buffer;
	response->head_len = 0;
	response->header = (char (*)[2048])(&buffer[128]);
	response->header_len = 0;
	response->body = (char (*)[128896])(&buffer[128 + 2048]);
	response->body_len = 0;
}

size_t response(request_t *req, response_t *res, char *buffer) {
	res->head_len += (uint8_t)sprintf(*res->head, "HTTP/1.1 %hu %s\r\n", res->status, status_text(res->status));
	if (res->header_len > 0) {
		memcpy(&buffer[res->head_len], res->header, res->header_len);
	}
	memcpy(&buffer[res->head_len + res->header_len], "\r\n", 2);
	res->header_len += 2;
	if (req->method_len == 4 && memcmp(req->method, "head", req->method_len) == 0) {
		res->body_len = 0;
	}
	return res->head_len + res->header_len + res->body_len;
}

void append_header(response_t *response, const char *format, ...) {
	va_list args;
	va_start(args, format);
	response->header_len += (uint16_t)vsprintf(*response->header + response->header_len, format, args);
	va_end(args);
}

void append_body(response_t *response, const void *buffer, size_t buffer_len) {
	memcpy(*response->body + response->body_len, buffer, buffer_len);
	response->body_len += buffer_len;
}
