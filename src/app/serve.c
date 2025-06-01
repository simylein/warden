#include "../lib/logger.h"
#include "../lib/response.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

file_t signin = {.fd = -1, .ptr = NULL, .len = 0, .name = "signin"};
file_t signup = {.fd = -1, .ptr = NULL, .len = 0, .name = "signup"};

file_t bad_request = {.fd = -1, .ptr = NULL, .len = 0, .name = "400"};
file_t unauthorized = {.fd = -1, .ptr = NULL, .len = 0, .name = "401"};
file_t forbidden = {.fd = -1, .ptr = NULL, .len = 0, .name = "403"};
file_t not_found = {.fd = -1, .ptr = NULL, .len = 0, .name = "404"};
file_t method_not_allowed = {.fd = -1, .ptr = NULL, .len = 0, .name = "405"};
file_t uri_too_long = {.fd = -1, .ptr = NULL, .len = 0, .name = "414"};
file_t request_header_fields_too_large = {.fd = -1, .ptr = NULL, .len = 0, .name = "431"};
file_t internal_server_error = {.fd = -1, .ptr = NULL, .len = 0, .name = "500"};
file_t http_version_not_supported = {.fd = -1, .ptr = NULL, .len = 0, .name = "505"};

void serve(file_t *asset, response_t *response) {
	char file_path[64];
	sprintf(file_path, "./src/app/%s.html", asset->name);

	if (file(file_path, asset) == -1) {
		response->status = 500;
		return;
	}

	if (asset->ptr != NULL) {
		info("sending file %s\n", asset->name);

		if (response->status == 0) {
			response->status = 200;
		}
		append_header(response, "content-type:%s\r\n", type(file_path));
		append_header(response, "content-length:%zu\r\n", asset->len);
		append_body(response, asset->ptr, asset->len);
	}
}
