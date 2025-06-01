#include "../lib/logger.h"
#include "../lib/response.h"
#include "file.h"
#include <stdio.h>
#include <string.h>

file_t signin = {.fd = -1, .ptr = NULL, .len = 0, .name = "signin"};
file_t signup = {.fd = -1, .ptr = NULL, .len = 0, .name = "signup"};

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
