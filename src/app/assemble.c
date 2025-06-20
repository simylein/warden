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
	bool inject = false;

	size_t asset_index = 0;
	size_t assemble_index = 0;

	size_t assemble_len = asset->len;
	char *assemble_ptr = malloc(assemble_len);
	if (assemble_ptr == NULL) {
		error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
		return -1;
	}

	while (asset_index < asset->len) {
		char *byte = &asset->ptr[asset_index];

		if (inject == false && asset_index + 11 < asset->len && memcmp(byte, "// @inject ", 11) == 0) {
			inject = true;
		}

		if (inject == true) {
			char path[65];
			uint8_t path_len = 0;
			byte += 11;
			asset_index += 11;
			char *start = byte;
			while (asset_index < asset->len) {
				if (*byte == '\n' || path_len >= sizeof(path) - 1) {
					inject = false;
					break;
				}
				byte += 1;
				path_len += 1;
				asset_index += 1;
			}
			sprintf(path, "%.*s", path_len, start);

			file_t assemble;
			assemble.fd = -1;
			assemble.ptr = NULL;
			assemble.path = path;

			if (file(&assemble) == -1) {
				return -1;
			}

			assemble_len += assemble.len;
			assemble_ptr = realloc(assemble_ptr, assemble_len);
			if (assemble_ptr == NULL) {
				error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
				return -1;
			}

			memcpy(&assemble_ptr[assemble_index], assemble.ptr, assemble.len);
			assemble_index += assemble.len;

			close(assemble.fd);
			free(assemble.ptr);
		} else {
			assemble_ptr[assemble_index] = *byte;
			assemble_index += 1;
		}

		asset_index += 1;
	}

	free(asset->ptr);

	asset->ptr = assemble_ptr;
	asset->len = assemble_len;

	return 0;
}
