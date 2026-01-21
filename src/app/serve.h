#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "file.h"

void serve(file_t *asset, response_t *response);

void serve_device(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_readings(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_metrics(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_buffers(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_config(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_radio(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_signals(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_uplinks(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_device_downlinks(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);

void serve_zone(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_readings(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_metrics(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_buffers(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void serve_zone_signals(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);

void serve_user(octet_t *db, request_t *request, response_t *response);
void serve_user_devices(octet_t *db, request_t *request, response_t *response);
void serve_user_zones(octet_t *db, request_t *request, response_t *response);
