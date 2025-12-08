#include "cache.h"
#include "../lib/config.h"
#include "../lib/logger.h"
#include "device.h"
#include "zone.h"
#include <pthread.h>
#include <stdint.h>
#include <string.h>

cache_t cache = {
		.devices_lock = PTHREAD_RWLOCK_INITIALIZER,
		.zones_lock = PTHREAD_RWLOCK_INITIALIZER,
};

int cache_device_read(cache_device_t *cache_device, device_t *device) {
	int status = -1;
	pthread_rwlock_rdlock(&cache.devices_lock);

	for (uint8_t index = 0; index < devices_size; index++) {
		cache_device_t *entry = &cache.devices[index];
		if (entry->name_len == 0) {
			break;
		}
		if (memcmp(entry->id, device->id, sizeof(entry->id)) == 0) {
			trace("read cache device at index %hhu\n", index);
			*cache_device = *entry;
			status = 0;
			break;
		}
	}

	pthread_rwlock_unlock(&cache.devices_lock);
	return status;
}

int cache_device_write(cache_device_t *cache_device) {
	int status = -1;
	pthread_rwlock_wrlock(&cache.devices_lock);

	for (uint8_t index = 0; index < devices_size; index++) {
		cache_device_t *entry = &cache.devices[index];
		if (entry->name_len == 0) {
			trace("write cache device at index %hhu\n", index);
			*entry = *cache_device;
			status = 0;
			break;
		}
		if (memcmp(entry->id, cache_device->id, sizeof(entry->id)) == 0) {
			trace("write cache device at index %hhu\n", index);
			*entry = *cache_device;
			status = 0;
			break;
		}
	}

	pthread_rwlock_unlock(&cache.devices_lock);
	return status;
}

int cache_zone_read(cache_zone_t *cache_zone, zone_t *zone) {
	int status = -1;
	pthread_rwlock_rdlock(&cache.zones_lock);

	for (uint8_t index = 0; index < zones_size; index++) {
		cache_zone_t *entry = &cache.zones[index];
		if (entry->name_len == 0) {
			break;
		}
		if (memcmp(entry->id, zone->id, sizeof(entry->id)) == 0) {
			trace("read cache zone at index %hhu\n", index);
			*cache_zone = *entry;
			status = 0;
			break;
		}
	}

	pthread_rwlock_unlock(&cache.zones_lock);
	return status;
}

int cache_zone_write(cache_zone_t *cache_zone) {
	int status = -1;
	pthread_rwlock_wrlock(&cache.zones_lock);

	for (uint8_t index = 0; index < zones_size; index++) {
		cache_zone_t *entry = &cache.zones[index];
		if (entry->name_len == 0) {
			trace("write cache zone at index %hhu\n", index);
			*entry = *cache_zone;
			status = 0;
			break;
		}
		if (memcmp(entry->id, cache_zone->id, sizeof(entry->id)) == 0) {
			trace("write cache zone at index %hhu\n", index);
			*entry = *cache_zone;
			status = 0;
			break;
		}
	}

	pthread_rwlock_unlock(&cache.zones_lock);
	return status;
}
