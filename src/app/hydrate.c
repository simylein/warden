#include "hydrate.h"
#include "../lib/error.h"
#include "../lib/logger.h"
#include "file.h"
#include "utility.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int append(class_t (*classes)[128], uint8_t *classes_len, char *next_ptr) {
	if (*classes_len + 1 > sizeof(*classes) / sizeof(class_t)) {
		error("can not handle more than %zu classes\n", sizeof(*classes) / sizeof(class_t));
		return -1;
	}

	if ((*classes)[*classes_len].len != 0) {
		bool unique = true;
		for (size_t index = 0; index < *classes_len; index++) {
			if ((*classes)[index].len == (*classes)[*classes_len].len &&
					memcmp((*classes)[index].ptr, (*classes)[*classes_len].ptr, (*classes)[index].len) == 0) {
				unique = false;
			}
		}
		if (unique) {
			(*classes_len)++;
		}
		(*classes)[*classes_len].len = 0;
	}

	if (next_ptr != NULL) {
		(*classes)[*classes_len].ptr = next_ptr;
	}

	return 0;
}

int extract(file_t *file, class_t (*classes)[128], uint8_t *classes_len) {
	bool tag = false;
	bool class = false;
	bool quote = false;

	(*classes)[*classes_len].len = 0;

	for (size_t index = 0; index < file->len; index++) {
		char *byte = &file->ptr[index];
		if (*byte == '<' && !tag && !quote) {
			tag = true;
		} else if (*byte == '>' && tag && !quote) {
			tag = false;
		} else if (*byte == '"' && tag && !quote) {
			quote = true;
			if (index > 12 && memcmp(&file->ptr[index - 6], "class=", 6) == 0) {
				if (append(classes, classes_len, &file->ptr[index + 1]) == -1) {
					return -1;
				}
				class = true;
			}
		} else if (*byte == '"' && tag && quote) {
			quote = false;
			if (class) {
				if (append(classes, classes_len, NULL) == -1) {
					return -1;
				}
				class = false;
			}
		} else if (tag && class && quote) {
			if (*byte == ' ') {
				if (append(classes, classes_len, &file->ptr[index + 1]) == -1) {
					return -1;
				}
			} else {
				(*classes)[*classes_len].len++;
			}
		}
	}

	return 0;
}

int hydrate(file_t *file, class_t (*classes)[128], uint8_t *classes_len) {
	char global[2048];
	uint16_t global_len = 0;
	breakpoint_t global_breakpoint = {.tag = "", .tag_len = 0};

	char small[2048];
	uint16_t small_len = 0;
	breakpoint_t small_breakpoint = {
			.tag = "sm", .tag_len = 2, .prefix = "@media (min-width:640px){", .prefix_len = 25, .suffix = "}", .suffix_len = 1};

	char medium[2048];
	uint16_t medium_len = 0;
	breakpoint_t medium_breakpoint = {
			.tag = "md", .tag_len = 2, .prefix = "@media (min-width:768px){", .prefix_len = 25, .suffix = "}", .suffix_len = 1};

	char large[2048];
	uint16_t large_len = 0;
	breakpoint_t large_breakpoint = {
			.tag = "lg", .tag_len = 2, .prefix = "@media (min-width:1024px){", .prefix_len = 26, .suffix = "}", .suffix_len = 1};

	debug("hydrating file %s\n", file->path);
	for (uint8_t index = 0; index < *classes_len; index++) {
		class_t pfx;
		class_t cls;

		uint8_t stg = 0;
		uint8_t ind = 0;

		if (memchr((*classes)[index].ptr, ':', (*classes)[index].len) == NULL) {
			stg = 1;
		}

		pfx.len = 0;
		pfx.ptr = &(*classes)[index].ptr[ind];
		while (stg == 0 && ind < (*classes)[index].len) {
			char *byte = &(*classes)[index].ptr[ind];
			if (*byte == ':') {
				stg = 1;
			} else {
				pfx.len++;
			}
			ind++;
		}

		cls.len = 0;
		cls.ptr = &(*classes)[index].ptr[ind];
		while (stg == 1 && ind < (*classes)[index].len) {
			cls.len++;
			ind++;
		}

		// printf("pfx %.*s cls %.*s\n", pfx.len, pfx.ptr, cls.len, cls.ptr);

		common(&pfx, &cls, &global, &global_len, &global_breakpoint);
		position(&pfx, &cls, &global, &global_len, &global_breakpoint);
		display(&pfx, &cls, &global, &global_len, &global_breakpoint);
		spacing(&pfx, &cls, &global, &global_len, &global_breakpoint);
		spacing(&pfx, &cls, &small, &small_len, &small_breakpoint);
		spacing(&pfx, &cls, &medium, &medium_len, &medium_breakpoint);
		spacing(&pfx, &cls, &large, &large_len, &large_breakpoint);
		sizing(&pfx, &cls, &global, &global_len, &global_breakpoint);
		sizing(&pfx, &cls, &small, &small_len, &small_breakpoint);
		sizing(&pfx, &cls, &medium, &medium_len, &medium_breakpoint);
		sizing(&pfx, &cls, &large, &large_len, &large_breakpoint);
	}

	size_t len = file->len + global_len + small_len + medium_len + large_len;
	if (small_len > 0) {
		len += small_breakpoint.prefix_len;
		len += small_breakpoint.suffix_len;
	}
	if (medium_len > 0) {
		len += medium_breakpoint.prefix_len;
		len += medium_breakpoint.suffix_len;
	}
	if (large_len > 0) {
		len += large_breakpoint.prefix_len;
		len += large_breakpoint.suffix_len;
	}
	char *ptr = malloc(len);
	if (ptr == NULL) {
		error("failed to allocate %zu bytes for %s because %s\n", len, file->path, errno_str());
		return -1;
	}

	bool tag = false;
	size_t ptr_ind = 0;
	size_t file_ind = 0;
	while (file_ind < file->len) {
		ptr[ptr_ind] = file->ptr[file_ind];
		if (file_ind > 13 && memcmp(&file->ptr[file_ind - 7], "<style>", 7) == 0) {
			tag = true;
			break;
		}
		ptr_ind++;
		file_ind++;
	}

	if (tag == false) {
		error("file %s does not contain a style tag\n", file->path);
		return -1;
	}

	if (global_len != 0) {
		memcpy(&ptr[ptr_ind], global, global_len);
		ptr_ind += global_len;
	}

	if (small_len != 0) {
		memcpy(&ptr[ptr_ind], small_breakpoint.prefix, small_breakpoint.prefix_len);
		ptr_ind += small_breakpoint.prefix_len;
		memcpy(&ptr[ptr_ind], small, small_len);
		ptr_ind += small_len;
		memcpy(&ptr[ptr_ind], small_breakpoint.suffix, small_breakpoint.suffix_len);
		ptr_ind += small_breakpoint.suffix_len;
	}

	if (medium_len != 0) {
		memcpy(&ptr[ptr_ind], medium_breakpoint.prefix, medium_breakpoint.prefix_len);
		ptr_ind += medium_breakpoint.prefix_len;
		memcpy(&ptr[ptr_ind], medium, medium_len);
		ptr_ind += medium_len;
		memcpy(&ptr[ptr_ind], medium_breakpoint.suffix, medium_breakpoint.suffix_len);
		ptr_ind += medium_breakpoint.suffix_len;
	}

	if (large_len != 0) {
		memcpy(&ptr[ptr_ind], large_breakpoint.prefix, large_breakpoint.prefix_len);
		ptr_ind += large_breakpoint.prefix_len;
		memcpy(&ptr[ptr_ind], large, large_len);
		ptr_ind += large_len;
		memcpy(&ptr[ptr_ind], large_breakpoint.suffix, large_breakpoint.suffix_len);
		ptr_ind += large_breakpoint.suffix_len;
	}

	memcpy(&ptr[ptr_ind], &file->ptr[file_ind], file->len - file_ind);
	ptr_ind += file->len - file_ind;
	file_ind += file->len - file_ind;

	free(file->ptr);
	file->ptr = ptr;
	file->len = len;
	file->hydrated = true;

	return 0;
}
