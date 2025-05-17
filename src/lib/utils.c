#include <stdint.h>
#include <stdlib.h>

const char *strncasestrn(const char *buffer, size_t buffer_len, const char *buf, size_t buf_len) {
	size_t index = 0;
	size_t ind = 0;

	while (index < buffer_len) {
		if (buffer[index] == buf[ind] || (buffer[index] >= 'A' && buffer[index] <= 'Z' && buffer[index] + 32 == buf[ind])) {
			ind++;
			if (ind == buf_len) {
				return (char *)&buffer[index + 1 - buf_len];
			}
		} else {
			ind = 0;
		}
		index++;
	}

	return NULL;
}
