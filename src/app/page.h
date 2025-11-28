#pragma once

#include "file.h"

extern file_t page_home;
extern file_t page_robots;
extern file_t page_security;
extern file_t page_devices;
extern file_t page_device;
extern file_t page_device_readings;
extern file_t page_device_metrics;
extern file_t page_device_buffers;
extern file_t page_device_signals;
extern file_t page_device_uplinks;
extern file_t page_device_downlinks;
extern file_t page_zones;
extern file_t page_zone;
extern file_t page_uplinks;
extern file_t page_uplink;
extern file_t page_downlinks;
extern file_t page_downlink;
extern file_t page_users;
extern file_t page_user;
extern file_t page_user_devices;
extern file_t page_profile;
extern file_t page_signin;
extern file_t page_signup;

extern file_t page_bad_request;
extern file_t page_unauthorized;
extern file_t page_forbidden;
extern file_t page_not_found;
extern file_t page_method_not_allowed;
extern file_t page_uri_too_long;
extern file_t page_request_header_fields_too_large;
extern file_t page_internal_server_error;
extern file_t page_service_unavailable;
extern file_t page_http_version_not_supported;
extern file_t page_insufficient_storage;

void page_init(void);
void page_close(void);
void page_free(void);
