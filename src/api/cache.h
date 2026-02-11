#pragma once

#include "device.h"
#include "zone.h"
#include <pthread.h>
#include <stdint.h>

typedef struct cache_device_t {
	uint8_t id[8];
	char name[16];
	uint8_t name_len;
	char zone_name[12];
	uint8_t zone_name_len;
} cache_device_t;

typedef struct cache_zone_t {
	uint8_t id[8];
	char name[12];
	uint8_t name_len;
} cache_zone_t;

typedef struct cache_t {
	cache_device_t *devices;
	pthread_rwlock_t devices_lock;
	cache_zone_t *zones;
	pthread_rwlock_t zones_lock;
} cache_t;

extern struct cache_t cache;

int cache_device_read(cache_device_t *cache_device, device_t *device);
int cache_device_write(cache_device_t *cache_device);

int cache_zone_read(cache_zone_t *cache_zone, zone_t *zone);
int cache_zone_write(cache_zone_t *cache_zone);
