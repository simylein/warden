#include "../api/device.h"
#include "../api/downlink.h"
#include "../api/uplink.h"
#include "../api/user.h"
#include "../lib/base16.h"
#include "../lib/bwt.h"
#include "../lib/logger.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include "assemble.h"
#include "file.h"
#include "hydrate.h"
#include "page.h"
#include <pthread.h>
#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>

void serve(file_t *asset, response_t *response) {
	if (file(asset) == -1) {
		response->status = 500;
		return;
	}

	pthread_rwlock_rdlock(&asset->lock);
	if (asset->hydrated == false) {
		pthread_rwlock_unlock(&asset->lock);
		pthread_rwlock_wrlock(&asset->lock);
		if (asset->hydrated == true) {
			goto cleanup;
		}

		if (assemble(asset) == -1) {
			response->status = 500;
			goto cleanup;
		}

		class_t classes[256];
		uint16_t classes_len = 0;
		if (extract(asset, &classes, &classes_len) == -1) {
			response->status = 500;
			goto cleanup;
		}

		if (hydrate(asset, &classes, &classes_len) == -1) {
			response->status = 500;
			goto cleanup;
		}

		pthread_rwlock_unlock(&asset->lock);
		pthread_rwlock_rdlock(&asset->lock);
	}

	if (asset->ptr != NULL) {
		info("sending file %s\n", asset->path);

		if (asset->len > response->body.cap) {
			error("file length %zu exceeds buffer length %u\n", asset->len, response->body.cap);
			response->status = 500;
			goto cleanup;
		}

		if (response->status == 0) {
			response->status = 200;
		}
		append_header(response, "content-type:%s\r\n", type(asset->path));
		append_header(response, "content-length:%zu\r\n", asset->len);
		append_body(response, asset->ptr, asset->len);
	}

cleanup:
	pthread_rwlock_unlock(&asset->lock);
}

void serve_device(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_device, response);
}

void serve_device_readings(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_device_readings, response);
}

void serve_device_metrics(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_device_metrics, response);
}

void serve_device_signals(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_device_signals, response);
}

void serve_device_uplinks(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_device_uplinks, response);
}

void serve_device_downlinks(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	device_t device = {.id = &id};
	uint16_t status = device_existing(database, bwt, &device);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_device_downlinks, response);
}

void serve_uplink(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 8, &uuid_len);
	if (uuid_len != sizeof(*((uplink_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((uplink_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	uplink_t uplink = {.id = &id};
	uint16_t status = uplink_existing(database, bwt, &uplink);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_uplink, response);
}

void serve_downlink(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 10, &uuid_len);
	if (uuid_len != sizeof(*((downlink_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((downlink_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	downlink_t downlink = {.id = &id};
	uint16_t status = downlink_existing(database, bwt, &downlink);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_downlink, response);
}

void serve_user(sqlite3 *database, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 6, &uuid_len);
	if (uuid_len != sizeof(*((downlink_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((downlink_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	user_t user = {.id = &id};
	uint16_t status = user_existing(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_user, response);
}

void serve_user_devices(sqlite3 *database, request_t *request, response_t *response) {
	uint8_t uuid_len = 0;
	const char *uuid = find_param(request, 6, &uuid_len);
	if (uuid_len != sizeof(*((downlink_t *)0)->id) * 2) {
		warn("uuid length %hhu does not match %zu\n", uuid_len, sizeof(*((downlink_t *)0)->id) * 2);
		response->status = 400;
		return;
	}

	uint8_t id[16];
	if (base16_decode(id, sizeof(id), uuid, uuid_len) != 0) {
		warn("failed to decode uuid from base 16\n");
		response->status = 400;
		return;
	}

	user_t user = {.id = &id};
	uint16_t status = user_existing(database, &user);
	if (status != 0) {
		response->status = status;
		return;
	}

	serve(&page_user_devices, response);
}
