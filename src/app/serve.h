#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "file.h"
#include <sqlite3.h>

void serve(file_t *asset, response_t *response);

void serve_home(request_t *request, response_t *response);
void serve_devices(request_t *request, response_t *response);
void serve_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_uplinks(request_t *request, response_t *response);
