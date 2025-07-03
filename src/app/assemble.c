#include "assemble.h"
#include "../lib/error.h"
#include "../lib/logger.h"
#include "file.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int assemble(file_t *asset) {
	size_t asset_ind = 0;
	size_t assemble_ind = 0;

	size_t assemble_len = 0;
	char *assemble_ptr = NULL;

	while (asset_ind < asset->len) {
		char *byte = &asset->ptr[asset_ind];

		if (asset_ind + 11 < asset->len && memcmp(byte, "// @inject ", 11) == 0) {
			char path[65];
			uint8_t path_len = 0;
			byte += 11;
			asset_ind += 11;
			char *start = byte;
			while (asset_ind < asset->len) {
				if (*byte == '\n' || path_len >= sizeof(path) - 1) {
					break;
				}
				byte += 1;
				path_len += 1;
				asset_ind += 1;
			}

			sprintf(path, "%.*s", path_len, start);
			file_t component = {.fd = -1, .ptr = NULL, .path = path, .lock = PTHREAD_RWLOCK_INITIALIZER};
			if (file(&component) == -1) {
				return -1;
			}

			assemble_len += component.len;
			assemble_ptr = realloc(assemble_ptr, assemble_len);
			if (assemble_ptr == NULL) {
				error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
				return -1;
			}

			memcpy(&assemble_ptr[assemble_ind], component.ptr, component.len);
			assemble_ind += component.len;
		} else {
			if (assemble_ind >= assemble_len) {
				assemble_len += 4096;
				assemble_ptr = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr == NULL) {
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
			}
			assemble_ptr[assemble_ind] = *byte;
			assemble_ind += 1;
		}

		asset_ind += 1;
	}

	free(asset->ptr);

	asset->ptr = assemble_ptr;
	asset->len = assemble_ind;

	return 0;
}
