#include "../app/serve.h"
#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include "uplink.h"
#include "user.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

bool pathcmp(const char *pattern, uint8_t pattern_len, const char *pathname, uint8_t pathname_len) {
	uint8_t pattern_ind = 0;
	uint8_t pathname_ind = 0;

	while (pattern_ind < pattern_len && pathname_ind < pathname_len) {
		if (pattern[pattern_ind] == ':') {
			while (pattern_ind < pattern_len && pattern[pattern_ind] != '/') {
				pattern_ind++;
			}
			while (pathname_ind < pathname_len && pathname[pathname_ind] != '/') {
				pathname_ind++;
			}
		} else if (pattern[pattern_ind] == pathname[pathname_ind]) {
			pattern_ind++;
			pathname_ind++;
		} else {
			return false;
		}
	}

	return pattern_ind == pattern_len && pathname_ind == pathname_len;
}

bool endpoint(request_t *request, const char *method, const char *pathname, bool *method_found, bool *pathname_found) {
	if (*method_found == true && *pathname_found == true) {
		return false;
	}

	if (pathcmp(pathname, (uint8_t)strlen(pathname), request->pathname, request->pathname_len) == true) {
		*pathname_found = true;

		if (strcmp(method, "get") == 0 && request->method_len == 4 && memcmp(request->method, "head", request->method_len) == 0) {
			*method_found = true;

			return true;
		}

		if (request->method_len == strlen(method) && memcmp(request->method, method, request->method_len) == 0) {
			*method_found = true;

			return true;
		}
	}

	return false;
}

bool authenticate(bool redirect, bwt_t *bwt, request_t *request, response_t *response) {
	const char *cookie = find_header(request, "cookie:");
	if (cookie == NULL) {
		if (redirect == true) {
			response->status = 307;
			append_header(response, "location:/signin\r\n");
			append_header(response, "set-cookie:memo=%.*s\r\n", (int)request->pathname_len, request->pathname);
		} else {
			response->status = 401;
		}
		return false;
	}

	if (bwt_verify(cookie, request->header_len - (size_t)(cookie - (const char *)request->header), bwt) == -1) {
		response->status = 401;
		return false;
	}

	return true;
}

void route(sqlite3 *database, request_t *request, response_t *response) {
	bool method_found = false;
	bool pathname_found = false;

	if (endpoint(request, "get", "/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&devices, response);
		}
	}

	if (endpoint(request, "get", "/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&uplinks, response);
		}
	}

	if (endpoint(request, "get", "/signin", &method_found, &pathname_found) == true) {
		serve(&signin, response);
	}

	if (endpoint(request, "get", "/signup", &method_found, &pathname_found) == true) {
		serve(&signup, response);
	}

	if (endpoint(request, "get", "/api/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			device_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "post", "/api/uplink", &method_found, &pathname_found) == true) {
		// FIXME authenticate and authorize this endpoint for nexus only
		uplink_create(database, request, response);
	}

	if (endpoint(request, "post", "/api/signin", &method_found, &pathname_found) == true) {
		user_signin(database, request, response);
	}

	if (endpoint(request, "post", "/api/signup", &method_found, &pathname_found) == true) {
		user_signup(database, request, response);
	}

	if (response->status == 0 && pathname_found == false && method_found == false) {
		response->status = 404;
	}
	if (response->status == 0 && pathname_found == true && method_found == false) {
		response->status = 405;
	}

	if (request->pathname_len >= 5 && memcmp(request->pathname, "/api/", 5) == 0) {
		return;
	}

	if (response->status == 400) {
		serve(&bad_request, response);
	}
	if (response->status == 401) {
		serve(&unauthorized, response);
	}
	if (response->status == 403) {
		serve(&forbidden, response);
	}
	if (response->status == 404) {
		serve(&not_found, response);
	}
	if (response->status == 405) {
		serve(&method_not_allowed, response);
	}
	if (response->status == 414) {
		serve(&uri_too_long, response);
	}
	if (response->status == 431) {
		serve(&request_header_fields_too_large, response);
	}
	if (response->status == 500) {
		serve(&internal_server_error, response);
	}
	if (response->status == 505) {
		serve(&http_version_not_supported, response);
	}
}
