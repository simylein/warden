#include "../app/page.h"
#include "../app/serve.h"
#include "../lib/bwt.h"
#include "../lib/endian.h"
#include "../lib/octet.h"
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
#include "user-zone.h"
#include "user.h"
#include "zone.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

const uint64_t permission_user_read = 1lu << 63lu;
const uint64_t permission_user_create = 1lu << 62lu;
const uint64_t permission_user_update = 1lu << 61lu;
const uint64_t permission_user_delete = 1lu << 60lu;
const uint64_t permission_user_device_read = 1lu << 59lu;
const uint64_t permission_user_device_create = 1lu << 58lu;
const uint64_t permission_user_device_update = 1lu << 57lu;
const uint64_t permission_user_device_delete = 1lu << 56lu;
const uint64_t permission_user_zone_read = 1lu << 55lu;
const uint64_t permission_user_zone_create = 1lu << 54lu;
const uint64_t permission_user_zone_update = 1lu << 53lu;
const uint64_t permission_user_zone_delete = 1lu << 52lu;
const uint64_t permission_device_read = 1lu << 51lu;
const uint64_t permission_device_create = 1lu << 50lu;
const uint64_t permission_device_update = 1lu << 49lu;
const uint64_t permission_device_delete = 1lu << 48lu;
const uint64_t permission_zone_read = 1lu << 47lu;
const uint64_t permission_zone_create = 1lu << 46lu;
const uint64_t permission_zone_update = 1lu << 45lu;
const uint64_t permission_zone_delete = 1lu << 44lu;
const uint64_t permission_config_read = 1lu << 43lu;
const uint64_t permission_config_create = 1lu << 42lu;
const uint64_t permission_config_update = 1lu << 41lu;
const uint64_t permission_config_delete = 1lu << 40lu;
const uint64_t permission_radio_read = 1lu << 39lu;
const uint64_t permission_radio_create = 1lu << 38lu;
const uint64_t permission_radio_update = 1lu << 37lu;
const uint64_t permission_radio_delete = 1lu << 36lu;
const uint64_t permission_uplink_read = 1lu << 7lu;
const uint64_t permission_uplink_create = 1lu << 6lu;
const uint64_t permission_uplink_update = 1lu << 5lu;
const uint64_t permission_uplink_delete = 1lu << 4lu;
const uint64_t permission_downlink_read = 1lu << 3lu;
const uint64_t permission_downlink_create = 1lu << 2lu;
const uint64_t permission_downlink_update = 1lu << 1lu;
const uint64_t permission_downlink_delete = 1lu << 0lu;

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

bool authorize(bwt_t *bwt, uint64_t permission, response_t *response) {
	uint64_t permissions;
	memcpy(&permissions, bwt->data, sizeof(bwt->data));
	permissions = ntoh64(permissions);

	if ((permissions & permission) != permission) {
		response->status = 403;
		return false;
	}

	return true;
}

void route(octet_t *db, request_t *request, response_t *response) {
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
			serve_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_readings(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_metrics(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_buffers(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/config", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_config(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/radio", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_radio(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_signals(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_uplinks(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/device/:id/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_device_downlinks(db, &bwt, request, response);
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
			serve_zone(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_readings(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_metrics(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_buffers(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/zone/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve_zone_signals(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_uplinks, response);
		}
	}

	if (endpoint(request, "get", "/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			serve(&page_downlinks, response);
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
				serve_user(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/user/:id/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_read, response) == true) {
				serve_user_devices(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/user/:id/zones", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(true, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_zone_read, response) == true) {
				serve_user_zones(db, request, response);
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
			device_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			buffer_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			device_find_one(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "patch", "/api/device/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_device_update, response) == true) {
				device_modify(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/device/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			buffer_find_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/config", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			config_find_one_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "patch", "/api/device/:id/config", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_config_update, response) == true) {
				config_modify(db, &bwt, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/device/:id/radio", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			radio_find_one_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "patch", "/api/device/:id/radio", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_radio_update, response) == true) {
				radio_modify(db, &bwt, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/device/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_signal_find_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_find_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/device/:id/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			downlink_find_by_device(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zones", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			zone_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			zone_find_one(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "patch", "/api/zone/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			zone_modify(db, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/readings", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			reading_find_by_zone(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/metrics", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			metric_find_by_zone(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/buffers", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			buffer_find_by_zone(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/zone/:id/signals", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_signal_find_by_zone(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "get", "/api/uplinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			uplink_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "post", "/api/uplink", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_uplink_create, response) == true) {
				uplink_create(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/downlinks", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			downlink_find(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "post", "/api/downlink", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_downlink_create, response) == true) {
				downlink_create(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/users", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				user_find(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/user/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_read, response) == true) {
				user_find_one(db, request, response);
			}
		}
	}

	if (endpoint(request, "delete", "/api/user/:id", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_delete, response) == true) {
				user_remove(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/user/:id/devices", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_read, response) == true) {
				device_find_by_user(db, request, response);
			}
		}
	}

	if (endpoint(request, "post", "/api/user/:id/device", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_create, response) == true) {
				user_device_create(db, request, response);
			}
		}
	}

	if (endpoint(request, "delete", "/api/user/:id/device", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_device_delete, response) == true) {
				user_device_remove(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/user/:id/zones", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_zone_read, response) == true) {
				zone_find_by_user(db, request, response);
			}
		}
	}

	if (endpoint(request, "post", "/api/user/:id/zone", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_zone_create, response) == true) {
				user_zone_create(db, request, response);
			}
		}
	}

	if (endpoint(request, "delete", "/api/user/:id/zone", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			if (authorize(&bwt, permission_user_zone_delete, response) == true) {
				user_zone_remove(db, request, response);
			}
		}
	}

	if (endpoint(request, "get", "/api/profile", &method_found, &pathname_found) == true) {
		bwt_t bwt;
		if (authenticate(false, &bwt, request, response) == true) {
			user_profile(db, &bwt, request, response);
		}
	}

	if (endpoint(request, "post", "/api/signin", &method_found, &pathname_found) == true) {
		user_signin(db, request, response);
	}

	if (endpoint(request, "post", "/api/signup", &method_found, &pathname_found) == true) {
		user_signup(db, request, response);
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
