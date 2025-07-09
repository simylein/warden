#include "../api/device.h"
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

		class_t classes[192];
		uint8_t classes_len = 0;
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

		if (asset->len > sizeof(*response->body)) {
			error("file length %zu exceeds buffer length %zu\n", asset->len, sizeof(*response->body));
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
	if (request->search_len != 0) {
		response->status = 400;
		return;
	}

	const char *uuid = *request->pathname + 8;
	size_t uuid_len = request->pathname_len - 8;
	if (uuid_len != sizeof(*((device_t *)0)->id) * 2) {
		warn("uuid length %zu does not match %zu\n", uuid_len, sizeof(*((device_t *)0)->id) * 2);
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
