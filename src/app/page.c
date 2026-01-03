#include "../lib/logger.h"
#include "file.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

file_t page_home = {.path = "./src/app/pages/home.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_robots = {.path = "./src/app/pages/robots.txt", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_security = {.path = "./src/app/pages/security.txt", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_devices = {.path = "./src/app/pages/devices.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device = {.path = "./src/app/pages/device.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_readings = {.path = "./src/app/pages/device-readings.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_metrics = {.path = "./src/app/pages/device-metrics.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_buffers = {.path = "./src/app/pages/device-buffers.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_config = {.path = "./src/app/pages/device-config.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_signals = {.path = "./src/app/pages/device-signals.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_uplinks = {.path = "./src/app/pages/device-uplinks.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_device_downlinks = {.path = "./src/app/pages/device-downlinks.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_zones = {.path = "./src/app/pages/zones.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_zone = {.path = "./src/app/pages/zone.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_zone_readings = {.path = "./src/app/pages/zone-readings.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_zone_metrics = {.path = "./src/app/pages/zone-metrics.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_zone_buffers = {.path = "./src/app/pages/zone-buffers.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_zone_signals = {.path = "./src/app/pages/zone-signals.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_uplinks = {.path = "./src/app/pages/uplinks.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_uplink = {.path = "./src/app/pages/uplink.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_downlinks = {.path = "./src/app/pages/downlinks.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_downlink = {.path = "./src/app/pages/downlink.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_users = {.path = "./src/app/pages/users.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_user = {.path = "./src/app/pages/user.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_user_devices = {.path = "./src/app/pages/user-devices.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_profile = {.path = "./src/app/pages/profile.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_signin = {.path = "./src/app/pages/signin.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_signup = {.path = "./src/app/pages/signup.html", .lock = PTHREAD_RWLOCK_INITIALIZER};

file_t page_bad_request = {.path = "./src/app/pages/400.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_unauthorized = {.path = "./src/app/pages/401.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_forbidden = {.path = "./src/app/pages/403.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_not_found = {.path = "./src/app/pages/404.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_method_not_allowed = {.path = "./src/app/pages/405.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_uri_too_long = {.path = "./src/app/pages/414.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_request_header_fields_too_large = {.path = "./src/app/pages/431.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_internal_server_error = {.path = "./src/app/pages/500.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_service_unavailable = {.path = "./src/app/pages/503.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_http_version_not_supported = {.path = "./src/app/pages/505.html", .lock = PTHREAD_RWLOCK_INITIALIZER};
file_t page_insufficient_storage = {.path = "./src/app/pages/507.html", .lock = PTHREAD_RWLOCK_INITIALIZER};

file_t *pages[] = {
		&page_home,
		&page_robots,
		&page_security,
		&page_devices,
		&page_device,
		&page_device_readings,
		&page_device_metrics,
		&page_device_buffers,
		&page_device_config,
		&page_device_signals,
		&page_device_uplinks,
		&page_device_downlinks,
		&page_zones,
		&page_zone,
		&page_zone_readings,
		&page_zone_metrics,
		&page_zone_buffers,
		&page_zone_signals,
		&page_uplinks,
		&page_uplink,
		&page_downlinks,
		&page_downlink,
		&page_users,
		&page_user,
		&page_user_devices,
		&page_profile,
		&page_signin,
		&page_signup,
		&page_bad_request,
		&page_unauthorized,
		&page_forbidden,
		&page_not_found,
		&page_method_not_allowed,
		&page_uri_too_long,
		&page_request_header_fields_too_large,
		&page_internal_server_error,
		&page_service_unavailable,
		&page_http_version_not_supported,
		&page_insufficient_storage,
};

void page_init(void) {
	trace("initialising %zu pages\n", sizeof(pages) / sizeof(*pages));

	for (uint8_t index = 0; index < sizeof(pages) / sizeof(*pages); index++) {
		pages[index]->fd = -1;
		pages[index]->ptr = NULL;
		pages[index]->len = 0;
		pages[index]->modified = 0;
		pages[index]->hydrated = false;
	}
}

void page_close(void) {
	uint8_t closed = 0;

	for (uint8_t index = 0; index < sizeof(pages) / sizeof(*pages); index++) {
		if (pages[index]->fd != -1) {
			close(pages[index]->fd);
			pages[index]->fd = -1;
			closed += 1;
		}
	}

	trace("closed %hhu pages\n", closed);
}

void page_free(void) {
	uint8_t freed = 0;

	for (uint8_t index = 0; index < sizeof(pages) / sizeof(*pages); index++) {
		if (pages[index]->ptr != NULL) {
			free(pages[index]->ptr);
			pages[index]->ptr = NULL;
			pages[index]->len = 0;
			pages[index]->modified = 0;
			pages[index]->hydrated = false;
			freed += 1;
		}
	}

	trace("freed %hhu pages\n", freed);
}
