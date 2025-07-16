#include <stdint.h>
#include <stdio.h>

int strnto16(const char *string, const size_t string_len, uint16_t *value) {
	for (size_t index = 0; index < string_len; index++) {
		if (string[index] < '0' || string[index] > '9') {
			return -1;
		}

		uint16_t digit = (uint16_t)string[index] - '0';
		*value = *value * 10 + digit;
	}

	return 0;
}

int strnto32(const char *string, const size_t string_len, uint32_t *value) {
	for (size_t index = 0; index < string_len; index++) {
		if (string[index] < '0' || string[index] > '9') {
			return -1;
		}

		uint32_t digit = (uint32_t)string[index] - '0';
		*value = *value * 10 + digit;
	}

	return 0;
}

int strnto64(const char *string, const size_t string_len, uint64_t *value) {
	for (size_t index = 0; index < string_len; index++) {
		if (string[index] < '0' || string[index] > '9') {
			return -1;
		}

		uint64_t digit = (uint64_t)string[index] - '0';
		*value = *value * 10 + digit;
	}

	return 0;
}
