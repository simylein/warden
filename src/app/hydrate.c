#include "hydrate.h"
#include "../lib/logger.h"
#include "file.h"
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
			if (index > 9 && memcmp(&file->ptr[index - 6], "class=", 6) == 0) {
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
