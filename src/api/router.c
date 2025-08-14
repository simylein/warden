#include "../app/page.h"
#include "../app/serve.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "device.h"
#include "downlink.h"
#include "metric.h"
#include "reading.h"
#include "recap.h"
#include "uplink.h"
#include "user.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

const uint32_t permission_user_read = 0x10000000;
const uint32_t permission_user_create = 0x20000000;
const uint32_t permission_user_update = 0x40000000;
const uint32_t permission_user_delete = 0x80000000;
const uint32_t permission_uplink_read = 0x00000010;
const uint32_t permission_uplink_create = 0x00000020;
const uint32_t permission_uplink_update = 0x00000040;
const uint32_t permission_uplink_delete = 0x00000080;
const uint32_t permission_downlink_read = 0x00000001;
const uint32_t permission_downlink_create = 0x00000002;
const uint32_t permission_downlink_update = 0x00000004;
const uint32_t permission_downlink_delete = 0x00000008;

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

	if (pathcmp(pathname, (uint8_t)strlen(pathname), *request->pathname, request->pathname_len) == true) {
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
	const char *cookie = find_header(request, "cookie");
	if (cookie == NULL) {
		if (redirect == true) {
			response->status = 307;
			append_header(response, "location:/signin\r\n");
			append_header(response, "set-cookie:memo=%.*s\r\n", (int)request->pathname_len, *request->pathname);
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

bool authorize(bwt_t *bwt, uint32_t permission, response_t *response) {
	uint32_t permissions;
	memcpy(&permissions, bwt->data, sizeof(bwt->data));
	permissions = ntoh32(permissions);

	if ((permissions & permission) != permission) {
		response->status = 403;
		return false;
	}

	return true;
}

void route(sqlite3 *database, request_t *request, response_t *response) {
	bool method_found = false;
	bool pathname_found = false;

	if (response->status != 0) {
		goto respond;
	}

	if (endpoint(request, "get", "/", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_home, response);
		}
	}

	if (endpoint(request, "get", "/robots.txt", &method_found, &pathname_found) == true) {
		serve(&page_robots, response);
	}

	if (endpoint(request, "get", "/security.txt", &method_found, &pathname_found) == true) {
		serve(&page_security, response);
	}

	if (endpoint(request, "get", "/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_devices, response);
		}
	}

	if (endpoint(request, "get", "/device/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_readings(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_metrics(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_signals(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_uplinks, response);
		}
	}

	if (endpoint(request, "get", "/uplink/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_uplink(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_downlinks, response);
		}
	}

	if (endpoint(request, "get", "/downlink/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_downlink(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/users", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				serve(&page_users, response);
			}
		}
	}

	if (endpoint(request, "get", "/user/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				serve_user(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/signin", &method_found, &pathname_found) == true) {
		serve(&page_signin, response);
	}

	if (endpoint(request, "get", "/signup", &method_found, &pathname_found) == true) {
		serve(&page_signup, response);
	}

	if (endpoint(request, "get", "/api/recap", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			recap_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			device_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			device_find_one(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find_by_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find_by_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_signal_find_by_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/uplink/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_find_one(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "post", "/api/uplink", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_uplink_create, response) == true) {
				uplink_create(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			downlink_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/downlink/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			downlink_find_one(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "post", "/api/downlink", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_downlink_create, response) == true) {
				downlink_create(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/users", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				user_find(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/user/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				user_find_one(database, request, response);
			}
		}
	}

	if (endpoint(request, "post", "/api/signin", &method_found, &pathname_found) == true) {
		user_signin(database, request, response);
	}

	if (endpoint(request, "post", "/api/signup", &method_found, &pathname_found) == true) {
		user_signup(database, request, response);
	}

respond:
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
		serve(&page_bad_request, response);
	}
	if (response->status == 401) {
		serve(&page_unauthorized, response);
	}
	if (response->status == 403) {
		serve(&page_forbidden, response);
	}
	if (response->status == 404) {
		serve(&page_not_found, response);
	}
	if (response->status == 405) {
		serve(&page_method_not_allowed, response);
	}
	if (response->status == 414) {
		serve(&page_uri_too_long, response);
	}
	if (response->status == 431) {
		serve(&page_request_header_fields_too_large, response);
	}
	if (response->status == 500) {
		serve(&page_internal_server_error, response);
	}
	if (response->status == 505) {
		serve(&page_http_version_not_supported, response);
	}
}
