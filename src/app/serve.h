#pragma once

#include "../lib/response.h"
#include "file.h"

extern file_t home;
extern file_t devices;
extern file_t uplinks;
extern file_t signin;
extern file_t signup;

extern file_t bad_request;
extern file_t unauthorized;
extern file_t forbidden;
extern file_t not_found;
extern file_t method_not_allowed;
extern file_t uri_too_long;
extern file_t request_header_fields_too_large;
extern file_t internal_server_error;
extern file_t http_version_not_supported;

void serve(file_t *asset, response_t *response);
