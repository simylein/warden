#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "file.h"
#include <sqlite3.h>

void serve(file_t *asset, response_t *response);

void serve_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_readings(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_metrics(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_buffers(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_signals(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_uplinks(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_downlinks(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);

void serve_zone(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_readings(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_metrics(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_buffers(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);

void serve_uplink(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);

void serve_downlink(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);

void serve_user(sqlite3 *database, request_t *request, response_t *response);
void serve_user_devices(sqlite3 *database, request_t *request, response_t *response);
