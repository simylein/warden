#include "../app/page.h"
#include "../app/serve.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "downlink.h"
#include "metric.h"
#include "radio.h"
#include "reading.h"
#include "uplink.h"
#include "user-device.h"
#include "user.h"
#include "zone.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

const uint32_t permission_user_read = 0x10000000;
const uint32_t permission_user_create = 0x20000000;
const uint32_t permission_user_update = 0x40000000;
const uint32_t permission_user_delete = 0x80000000;
const uint32_t permission_user_device_read = 0x01000000;
const uint32_t permission_user_device_create = 0x02000000;
const uint32_t permission_user_device_update = 0x04000000;
const uint32_t permission_user_device_delete = 0x08000000;
const uint32_t permission_device_read = 0x00100000;
const uint32_t permission_device_create = 0x00200000;
const uint32_t permission_device_update = 0x00400000;
const uint32_t permission_device_delete = 0x00800000;
const uint32_t permission_zone_read = 0x00010000;
const uint32_t permission_zone_create = 0x00020000;
const uint32_t permission_zone_update = 0x00040000;
const uint32_t permission_zone_delete = 0x00080000;
const uint32_t permission_config_read = 0x00001000;
const uint32_t permission_config_create = 0x00002000;
const uint32_t permission_config_update = 0x00004000;
const uint32_t permission_config_delete = 0x00008000;
const uint32_t permission_radio_read = 0x00000100;
const uint32_t permission_radio_create = 0x00000200;
const uint32_t permission_radio_update = 0x00000400;
const uint32_t permission_radio_delete = 0x00000800;
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

	if (pathcmp(pathname, (uint8_t)strlen(pathname), request->pathname.ptr, (uint8_t)request->pathname.len) == true) {
		*pathname_found = true;

		if (strcmp(method, "get") == 0 && request->method.len == 4 &&
				memcmp(request->method.ptr, "head", request->method.len) == 0) {
			*method_found = true;

			return true;
		}

		if (request->method.len == strlen(method) && memcmp(request->method.ptr, method, request->method.len) == 0) {
			*method_found = true;

			return true;
		}
	}

	return false;
}

bool authenticate(bool redirect, bwt_t *bwt, request_t *request, response_t *response) {
	const char *cookie = header_find(request, "cookie");
	if (cookie == NULL) {
		if (redirect == true) {
			response->status = 307;
			header_write(response, "location:/signin\r\n");
			header_write(response, "set-cookie:memo=%.*s\r\n", (int)request->pathname.len, request->pathname.ptr);
		} else {
			response->status = 401;
		}
		return false;
	}

	if (bwt_verify(cookie, request->header.len - (size_t)(cookie - (const char *)request->header.ptr), bwt) == -1) {
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

void route(const char *db, sqlite3 *database, request_t *request, response_t *response) {
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

	if (endpoint(request, "get", "/device/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_buffers(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/config", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_config(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/radio", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_radio(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_signals(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_uplinks(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_downlinks(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zones", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_zones, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_readings(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_metrics(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_buffers(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_signals(database, &bwt, request, response);
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

	if (endpoint(request, "get", "/user/:id/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_read, response) == true) {
				serve_user_devices(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/profile", &method_found, &pathname_found)) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_profile, response);
		}
	}

	if (endpoint(request, "get", "/signin", &method_found, &pathname_found) == true) {
		serve(&page_signin, response);
	}

	if (endpoint(request, "get", "/signup", &method_found, &pathname_found) == true) {
		serve(&page_signup, response);
	}

	if (endpoint(request, "get", "/api/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			device_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			buffer_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			device_find_one(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "patch", "/api/device/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_device_update, response) == true) {
				device_modify(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/device/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find_by_device(db, database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find_by_device(db, database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			buffer_find_by_device(db, database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/config", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			config_find_one_by_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/radio", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			radio_find_one_by_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_signal_find_by_device(db, database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_find_by_device(db, database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			downlink_find_by_device(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zones", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			zone_find(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			zone_find_one(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "patch", "/api/zone/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			zone_modify(database, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find_by_zone(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find_by_zone(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			buffer_find_by_zone(database, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_signal_find_by_zone(database, &bwt, request, response);
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
				uplink_create(db, database, request, response);
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
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				user_find(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/user/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				user_find_one(database, request, response);
			}
		}
	}

	if (endpoint(request, "delete", "/api/user/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_delete, response) == true) {
				user_remove(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/user/:id/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_read, response) == true) {
				device_find_by_user(database, request, response);
			}
		}
	}

	if (endpoint(request, "post", "/api/user/:id/device", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_create, response) == true) {
				user_device_create(database, request, response);
			}
		}
	}

	if (endpoint(request, "delete", "/api/user/:id/device", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_delete, response) == true) {
				user_device_remove(database, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/profile", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			user_profile(database, &bwt, request, response);
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

	if (request->pathname.len >= 5 && memcmp(request->pathname.ptr, "/api/", 5) == 0) {
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
	if (response->status == 503) {
		serve(&page_service_unavailable, response);
	}
	if (response->status == 505) {
		serve(&page_http_version_not_supported, response);
	}
	if (response->status == 507) {
		serve(&page_insufficient_storage, response);
	}
}
