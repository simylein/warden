#include "request.h"
#include "response.h"
#include "strn.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void request_init(request_t *request) {
	request->method_len = 0;
	request->pathname_len = 0;
	request->search_len = 0;
	request->protocol_len = 0;
	request->header_len = 0;
	request->body_len = 0;
}

void request(char *buffer, size_t length, request_t *req, response_t *res) {
	uint8_t stage = 0;
	size_t index = 0;

	const size_t method_index = index;
	while (stage == 0 && req->method_len < sizeof(*req->method) && index < length) {
		char *byte = &buffer[index];
		if (*byte >= 'A' && *byte <= 'Z') {
			*byte += 32;
		}
		if (*byte == ' ') {
			stage = 1;
		} else if (*byte <= '\037') {
			res->status = 400;
			return;
		} else {
			req->method_len++;
		}
		index++;
	}
	req->method = (char (*)[sizeof(*req->method)])(&buffer[method_index]);
	if (stage == 0 || req->method_len == 0) {
		res->status = 501;
		return;
	}

	const size_t pathname_index = index;
	while (stage == 1 && req->pathname_len < sizeof(*req->pathname) && index < length) {
		char *byte = &buffer[index];
		if (*byte == '?') {
			stage = 2;
		} else if (*byte == ' ') {
			stage = 3;
		} else if (*byte <= '\037') {
			res->status = 400;
			return;
		} else {
			req->pathname_len++;
		}
		index++;
	}
	req->pathname = (char (*)[sizeof(*req->pathname)])(&buffer[pathname_index]);
	if (stage == 1 || req->pathname_len == 0) {
		res->status = 414;
		return;
	}

	const size_t search_index = index;
	while (stage == 2 && req->search_len < sizeof(*req->search) && index < length) {
		char *byte = &buffer[index];
		if (*byte == ' ') {
			stage = 3;
		} else if (*byte <= '\037') {
			res->status = 400;
			return;
		} else {
			req->search_len++;
		}
		index++;
	}
	req->search = (char (*)[sizeof(*req->search)])(&buffer[search_index]);
	if (stage == 2) {
		res->status = 414;
		return;
	}

	const size_t protocol_index = index;
	while ((stage == 3 || stage == 4) && req->protocol_len < sizeof(*req->protocol) && index < length) {
		char *byte = &buffer[index];
		if (*byte >= 'A' && *byte <= 'Z') {
			*byte += 32;
		}
		if (*byte == '\r' || *byte == '\n') {
			stage += 1;
		} else if (*byte <= '\037') {
			res->status = 400;
			return;
		} else {
			req->protocol_len++;
		}
		index++;
	}
	req->protocol = (char (*)[sizeof(*req->protocol)])(&buffer[protocol_index]);
	if (stage == 3 || req->protocol_len == 0) {
		res->status = 505;
		return;
	}
	if (stage == 4 || req->protocol_len == 0) {
		res->status = 400;
		return;
	}

	bool header_key = true;
	const size_t header_index = index;
	while ((stage >= 3 && stage <= 6) && req->header_len < sizeof(*req->header) && index < length) {
		char *byte = &buffer[index];
		if (header_key == true && *byte >= 'A' && *byte <= 'Z') {
			*byte += 32;
		}
		if (*byte == ':') {
			header_key = false;
		}
		if (*byte == '\r' || *byte == '\n') {
			header_key = true;
			stage += 1;
		} else if (*byte <= '\037') {
			res->status = 400;
			return;
		} else {
			stage = 3;
		}
		req->header_len++;
		index++;
	}
	req->header = (char (*)[sizeof(*req->header)])(&buffer[header_index]);
	if (stage >= 3 && stage <= 5) {
		res->status = 431;
		return;
	}
	if (stage == 6) {
		res->status = 400;
		return;
	}

	req->body_len = length - index;
	req->body = (char (*)[sizeof(*req->body)])(&buffer[index]);
}

const char *find_param(request_t *request, uint8_t offset, uint8_t *length) {
	if (request->pathname_len < offset) {
		return NULL;
	}

	while (offset + *length < request->pathname_len) {
		if ((*request->pathname)[offset + *length] == '/') {
			break;
		}
		*length += 1;
	}

	return &(*request->pathname)[offset];
}

const char *find_header(request_t *request, const char *key) {
	const char *header = strncasestrn(*request->header, request->header_len, key, strlen(key));
	if (header == NULL) {
		return NULL;
	}

	header += strlen(key);
	if (header[0] == ':') {
		header += 1;
	}
	if (header[0] == ' ') {
		header += 1;
	}

	return header;
}
