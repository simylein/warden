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
	inject_t injects[16];
	uint8_t injects_len = 0;

	bool inject = false;

	uint8_t stage = 0;
	size_t index = 0;

	size_t script_index;
	while (stage == 0 && index < asset->len) {
		if (index > 14 && memcmp(&asset->ptr[index - 8], "<script>", 8) == 0) {
			script_index = index;
			stage = 1;
		}
		index++;
	}

	while (stage == 1 && index < asset->len) {
		char *byte = &asset->ptr[index];
		if (inject == false && index > 17 && memcmp(&asset->ptr[index - 11], "// @inject ", 11) == 0) {
			injects[injects_len].ptr = &asset->ptr[index];
			injects[injects_len].len = 0;
			inject = true;
		} else if (inject == true && *byte == '\n') {
			injects[injects_len].len += 1;
			injects_len += 1;
			if (injects_len >= sizeof(injects) / sizeof(inject_t)) {
				error("can not assemble more than %zu files\n", sizeof(injects) / sizeof(inject_t));
				return -1;
			}
			inject = false;
		} else if (inject == true) {
			injects[injects_len].len += 1;
		} else if (index > 15 && memcmp(&asset->ptr[index - 9], "</script>", 9) == 0) {
			stage = 2;
		}
		index++;
	}

	if (stage == 0) {
		return 0;
	}

	if (stage != 2) {
		error("file %s contains an invalid script tag\n", asset->path);
		return -1;
	}

	file_t files[16];
	uint8_t files_len = 0;

	for (uint8_t ind = 0; ind < injects_len; ind++) {
		char path[65];
		if (injects[ind].len > sizeof(path) - 1) {
			error("inject %.*s exceeds buffer size %zu\n", injects[ind].len, injects[ind].ptr, sizeof(path) - 1);
			return -1;
		}
		files[ind].fd = -1;
		files[ind].ptr = NULL;
		sprintf(path, "%.*s", injects[ind].len, injects[ind].ptr);
		files[ind].path = path;
		if (file(&files[files_len]) == -1) {
			return -1;
		}
		files_len += 1;
	}

	size_t assemble_len = asset->len;
	for (uint8_t ind = 0; ind < files_len; ind++) {
		assemble_len += files[ind].len;
	}

	char *assemble_ptr = malloc(assemble_len);
	if (assemble_ptr == NULL) {
		error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
		return -1;
	}

	size_t asset_index = 0;
	size_t assemble_index = 0;

	memcpy(&assemble_ptr[assemble_index], &asset->ptr[asset_index], script_index);
	asset_index += script_index;
	assemble_index += script_index;

	for (uint8_t ind = 0; ind < files_len; ind++) {
		memcpy(&assemble_ptr[assemble_index], files[ind].ptr, files[ind].len);
		assemble_index += files[ind].len;
	}

	memcpy(&assemble_ptr[assemble_index], &asset->ptr[asset_index], asset->len - script_index);
	asset_index += asset->len - script_index;
	assemble_index += asset->len - script_index;

	asset->ptr = assemble_ptr;
	asset->len = assemble_len;

	for (uint8_t ind = 0; ind < files_len; ind++) {
		close(files[ind].fd);
		free(files[ind].ptr);
	}

	return 0;
}
