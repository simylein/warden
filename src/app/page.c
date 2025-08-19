#include "file.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

file_t page_home = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/home.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_robots = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/robots.txt",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_security = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/security.txt",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_devices = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/devices.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_device = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/device.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_device_readings = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/device-readings.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_device_metrics = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/device-metrics.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_device_signals = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/device-signals.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_uplinks = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/uplinks.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_uplink = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/uplink.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_downlinks = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/downlinks.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_downlink = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/downlink.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_users = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/users.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_user = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/user.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_user_devices = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/user-devices.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_signin = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/signin.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_signup = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/pages/signup.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};

file_t page_bad_request = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/400.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_unauthorized = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/401.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_forbidden = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/403.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_not_found = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/404.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_method_not_allowed = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/405.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_uri_too_long = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/414.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_request_header_fields_too_large = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/431.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_internal_server_error = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/500.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};
file_t page_http_version_not_supported = {
		.fd = -1,
		.ptr = NULL,
		.len = 0,
		.path = "./src/app/errors/505.html",
		.modified = 0,
		.hydrated = false,
		.lock = PTHREAD_RWLOCK_INITIALIZER,
};

void page_close(void) {
	close(page_home.fd);
	close(page_robots.fd);
	close(page_security.fd);
	close(page_devices.fd);
	close(page_device.fd);
	close(page_device_readings.fd);
	close(page_device_metrics.fd);
	close(page_device_signals.fd);
	close(page_uplinks.fd);
	close(page_uplink.fd);
	close(page_downlinks.fd);
	close(page_downlink.fd);
	close(page_users.fd);
	close(page_user.fd);
	close(page_user_devices.fd);
	close(page_signin.fd);
	close(page_signup.fd);

	close(page_bad_request.fd);
	close(page_unauthorized.fd);
	close(page_forbidden.fd);
	close(page_not_found.fd);
	close(page_method_not_allowed.fd);
	close(page_uri_too_long.fd);
	close(page_request_header_fields_too_large.fd);
	close(page_internal_server_error.fd);
	close(page_http_version_not_supported.fd);
}

void page_free(void) {
	free(page_home.ptr);
	free(page_robots.ptr);
	free(page_security.ptr);
	free(page_devices.ptr);
	free(page_device.ptr);
	free(page_device_readings.ptr);
	free(page_device_metrics.ptr);
	free(page_device_signals.ptr);
	free(page_uplinks.ptr);
	free(page_uplink.ptr);
	free(page_downlinks.ptr);
	free(page_downlink.ptr);
	free(page_users.ptr);
	free(page_user.ptr);
	free(page_user_devices.ptr);
	free(page_signin.ptr);
	free(page_signup.ptr);

	free(page_bad_request.ptr);
	free(page_unauthorized.ptr);
	free(page_forbidden.ptr);
	free(page_not_found.ptr);
	free(page_method_not_allowed.ptr);
	free(page_uri_too_long.ptr);
	free(page_request_header_fields_too_large.ptr);
	free(page_internal_server_error.ptr);
	free(page_http_version_not_supported.ptr);
}
