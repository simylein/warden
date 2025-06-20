#include "../lib/logger.h"
#include "../lib/response.h"
#include "assemble.h"
#include "file.h"
#include "hydrate.h"
#include <stdbool.h>
#include <stdio.h>

file_t signin = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/signin.html", .modified = 0, .hydrated = false};
file_t signup = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/signup.html", .modified = 0, .hydrated = false};
file_t devices = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/devices.html", .modified = 0, .hydrated = false};
file_t uplinks = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/uplinks.html", .modified = 0, .hydrated = false};

file_t bad_request = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/400.html", .modified = 0, .hydrated = false};
file_t unauthorized = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/401.html", .modified = 0, .hydrated = false};
file_t forbidden = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/403.html", .modified = 0, .hydrated = false};
file_t not_found = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/404.html", .modified = 0, .hydrated = false};
file_t method_not_allowed = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/405.html", .modified = 0, .hydrated = false};
file_t uri_too_long = {.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/414.html", .modified = 0, .hydrated = false};
file_t request_header_fields_too_large = {
		.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/431.html", .modified = 0, .hydrated = false};
file_t internal_server_error = {
		.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/500.html", .modified = 0, .hydrated = false};
file_t http_version_not_supported = {
		.fd = -1, .ptr = NULL, .len = 0, .path = "./src/app/505.html", .modified = 0, .hydrated = false};

void serve(file_t *asset, response_t *response) {
	if (file(asset) == -1) {
		response->status = 500;
		return;
	}

	if (asset->hydrated == false) {
		if (assemble(asset) == -1) {
			response->status = 500;
			return;
		}

		class_t classes[128];
		uint8_t classes_len = 0;
		if (extract(asset, &classes, &classes_len) == -1) {
			response->status = 500;
			return;
		}

		if (hydrate(asset, &classes, &classes_len) == -1) {
			response->status = 500;
			return;
		}
	}

	if (asset->ptr != NULL) {
		info("sending file %s\n", asset->path);

		if (response->status == 0) {
			response->status = 200;
		}
		append_header(response, "content-type:%s\r\n", type(asset->path));
		append_header(response, "content-length:%zu\r\n", asset->len);
		append_body(response, asset->ptr, asset->len);
	}
}
