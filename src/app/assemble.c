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
	bool recurse = false;

	bool tag = false;
	bool quote = false;

	size_t asset_ind = 0;
	size_t assemble_ind = 0;

	size_t assemble_len = 0;
	char *assemble_ptr = NULL;

recursion:
	while (asset_ind < asset->len) {
		char *byte = &asset->ptr[asset_ind];
		if (*byte == '<' && !tag && !quote) {
			if (asset_ind + 7 < asset->len && memcmp(byte + 1, "/body>", 6) == 0) {
				break;
			}
			tag = true;
		} else if (*byte == '>' && tag && !quote) {
			tag = false;
		} else if (*byte == '"' && tag && !quote) {
			quote = true;
		} else if (*byte == '"' && tag && quote) {
			quote = false;
		}

		if (tag && !quote && *byte == ' ' && asset_ind + 6 < asset->len && memcmp(byte + 1, "ref=\"", 5) == 0) {
			char path[65];
			uint8_t path_len = 0;
			byte += 6;
			asset_ind += 6;
			char *start = byte;
			while (asset_ind < asset->len) {
				if (*byte == '"' || path_len >= sizeof(path) - 1) {
					break;
				}
				byte += 1;
				path_len += 1;
				asset_ind += 1;
			}
			while (asset_ind < asset->len) {
				if (*byte == '>') {
					break;
				}
				byte += 1;
				asset_ind += 1;
			}

			sprintf(path, "%.*s", path_len, start);
			file_t component = {.fd = -1, .ptr = NULL, .path = path, .lock = PTHREAD_RWLOCK_INITIALIZER};
			if (file(&component) == -1) {
				return -1;
			}

			if (assemble_ind + component.len >= assemble_len) {
				assemble_len += component.len;
				char *assemble_ptr_new = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr_new == NULL) {
					close(component.fd);
					free(component.ptr);
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
				assemble_ptr = assemble_ptr_new;
			}

			while (assemble_ind > 0 && assemble_ptr[assemble_ind] != '<') {
				assemble_ind -= 1;
			}

			memcpy(&assemble_ptr[assemble_ind], component.ptr, component.len);
			assemble_ind += component.len;

			for (size_t ind = 0; ind + 6 <= component.len; ind++) {
				if (component.ptr[ind] == ' ' && memcmp(&component.ptr[ind + 1], "ref=\"", 5) == 0) {
					recurse = true;
					break;
				}
			}

			close(component.fd);
			free(component.ptr);
		} else if (!tag && !quote && *byte == '{' && asset_ind + 9 < asset->len && memcmp(byte + 1, "version}", 8) == 0) {
			byte += 8;
			asset_ind += 8;

			size_t version_len = strlen(version);
			if (assemble_ind + version_len >= assemble_len) {
				assemble_len += version_len;
				char *assemble_ptr_new = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr_new == NULL) {
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
				assemble_ptr = assemble_ptr_new;
			}
			memcpy(&assemble_ptr[assemble_ind], version, version_len);
			assemble_ind += version_len;
		} else if (!tag && !quote && *byte == '{' && asset_ind + 8 < asset->len && memcmp(byte + 1, "commit}", 7) == 0) {
			byte += 7;
			asset_ind += 7;

			size_t commit_len = strlen(commit);
			if (assemble_ind + commit_len >= assemble_len) {
				assemble_len += commit_len;
				char *assemble_ptr_new = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr_new == NULL) {
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
				assemble_ptr = assemble_ptr_new;
			}
			memcpy(&assemble_ptr[assemble_ind], commit, commit_len);
			assemble_ind += commit_len;
		} else {
			if (assemble_ind >= assemble_len) {
				assemble_len += 4096;
				char *assemble_ptr_new = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr_new == NULL) {
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
				assemble_ptr = assemble_ptr_new;
			}
			assemble_ptr[assemble_ind] = *byte;
			assemble_ind += 1;
		}

		asset_ind += 1;
	}

	while (asset_ind < asset->len) {
		char *byte = &asset->ptr[asset_ind];

		if (*byte == 'i' && asset_ind + 8 < asset->len && memcmp(byte + 1, "mport '", 7) == 0) {
			char path[65];
			uint8_t path_len = 0;
			byte += 8;
			asset_ind += 8;
			char *start = byte;
			while (asset_ind < asset->len) {
				if (*byte == '\n' || path_len >= sizeof(path) - 1) {
					break;
				}
				byte += 1;
				if (*byte != '\'' && *byte != ';') {
					path_len += 1;
				}
				asset_ind += 1;
			}

			sprintf(path, "%.*s", path_len, start);
			file_t script = {.fd = -1, .ptr = NULL, .path = path, .lock = PTHREAD_RWLOCK_INITIALIZER};
			if (file(&script) == -1) {
				return -1;
			}

			if (assemble_ind + script.len >= assemble_len) {
				assemble_len += script.len;
				char *assemble_ptr_new = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr_new == NULL) {
					close(script.fd);
					free(script.ptr);
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
				assemble_ptr = assemble_ptr_new;
			}

			memcpy(&assemble_ptr[assemble_ind], script.ptr, script.len);
			assemble_ind += script.len;

			close(script.fd);
			free(script.ptr);
		} else {
			if (assemble_ind >= assemble_len) {
				assemble_len += 4096;
				char *assemble_ptr_new = realloc(assemble_ptr, assemble_len);
				if (assemble_ptr_new == NULL) {
					error("failed to allocate %zu bytes for %s because %s\n", assemble_len, asset->path, errno_str());
					return -1;
				}
				assemble_ptr = assemble_ptr_new;
			}
			assemble_ptr[assemble_ind] = *byte;
			assemble_ind += 1;
		}

		asset_ind += 1;
	}

	free(asset->ptr);

	asset->ptr = assemble_ptr;
	asset->len = assemble_ind;

	if (recurse == true) {
		debug("recursing file %s\n", asset->path);

		recurse = false;

		tag = false;
		quote = false;

		asset_ind = 0;
		assemble_ind = 0;

		assemble_len = 0;
		assemble_ptr = NULL;

		goto recursion;
	}

	return 0;
}
