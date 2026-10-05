#pragma once

#include "../lib/bwt.h"
#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "buffer.h"
#include "downlink.h"
#include "metric.h"
#include "reading.h"
#include "uplink.h"
#include "user.h"
#include "zone.h"
#include <stdint.h>
#include <time.h>

typedef struct uplink_t uplink_t;
typedef struct downlink_t downlink_t;

typedef struct device_t {
	uint8_t (*id)[8];
	char *name;
	uint8_t name_len;
	uint8_t (*zone_id)[8];
	char *zone_name;
	uint8_t zone_name_len;
	uint8_t (*zone_color)[12];
	char *firmware;
	uint8_t firmware_len;
	char *hardware;
	uint8_t hardware_len;
	time_t *created_at;
	time_t *updated_at;
	reading_t *reading;
	metric_t *metric;
	buffer_t *buffer;
	uplink_t *uplink;
	downlink_t *downlink;
} device_t;

typedef struct device_query_t {
	const char *order;
	size_t order_len;
	const char *sort;
	size_t sort_len;
	uint8_t limit;
	uint32_t offset;
} device_query_t;

typedef struct device_row_t {
	uint16_t id;
	uint16_t name_len;
	uint16_t name;
	uint16_t firmware_len;
	uint16_t firmware;
	uint16_t hardware_len;
	uint16_t hardware;
	uint16_t created_at;
	uint16_t updated_at_null;
	uint16_t updated_at;
	uint16_t airtime;
	uint16_t airtime_bucket;
	uint16_t packet_rx;
	uint16_t packet_lost;
	uint16_t packet_bucket;
	uint16_t zone_null;
	uint16_t zone_id;
	uint16_t zone_name_len;
	uint16_t zone_name;
	uint16_t zone_color;
	uint16_t reading_null;
	uint16_t reading_temperature;
	uint16_t reading_humidity;
	uint16_t reading_dewpoint;
	uint16_t reading_captured_at;
	uint16_t metric_null;
	uint16_t metric_photovoltaic;
	uint16_t metric_battery;
	uint16_t metric_captured_at;
	uint16_t buffer_null;
	uint16_t buffer_delay;
	uint16_t buffer_level;
	uint16_t buffer_captured_at;
	uint16_t uplink_null;
	uint16_t uplink_frame;
	uint16_t uplink_kind;
	uint16_t uplink_rssi;
	uint16_t uplink_snr;
	uint16_t uplink_sf;
	uint16_t uplink_received_at;
	uint16_t downlink_null;
	uint16_t downlink_frame;
	uint16_t downlink_kind;
	uint16_t downlink_sf;
	uint16_t downlink_cr;
	uint16_t downlink_tx_power;
	uint16_t downlink_sent_at;
	uint16_t size;
} device_row_t;

extern const char *device_file;

extern const device_row_t device_row;

uint16_t device_existing(octet_t *db, device_t *device);

uint16_t device_select(octet_t *db, bwt_t *bwt, device_query_t *query, response_t *response, uint8_t *devices_len);
uint16_t device_select_one(octet_t *db, bwt_t *bwt, device_t *device, response_t *response);
uint16_t device_select_by_user(octet_t *db, user_t *user, device_query_t *query, response_t *response, uint8_t *devices_len);
uint16_t device_select_by_zone(octet_t *db, zone_t *zone, uint8_t *devices_len);
uint16_t device_insert(octet_t *db, device_t *device);
uint16_t device_update(octet_t *db, device_t *device);
uint16_t device_update_zones(octet_t *db, zone_t *zone);
uint16_t device_update_latest(octet_t *db, device_t *device);

void device_find(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void device_find_one(octet_t *db, bwt_t *bwt, request_t *request, response_t *response);
void device_find_by_user(octet_t *db, request_t *request, response_t *response);
void device_modify(octet_t *db, request_t *request, response_t *response);
