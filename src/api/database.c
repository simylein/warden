#include "../lib/logger.h"
#include <sqlite3.h>
#include <stdint.h>

uint16_t database_error(sqlite3 *database, int result) {
	uint16_t status;

	if (result == SQLITE_BUSY || result == SQLITE_LOCKED || result == SQLITE_NOMEM) {
		status = 503;
	} else if (result == SQLITE_FULL) {
		status = 507;
	} else {
		status = 500;
	}

	error("failed to execute statement because %s\n", sqlite3_errmsg(database));
	return status;
}

uint8_t database_uint8(uint8_t *row, uint8_t row_ind) {
	uint8_t value = row[row_ind];
	return value;
}

uint16_t database_uint16(uint8_t *row, uint8_t row_ind) {
	uint16_t value = (uint16_t)(((uint16_t)(row[row_ind]) << 8) | (uint16_t)row[row_ind + 1]);
	return value;
}

uint32_t database_uint32(uint8_t *row, uint8_t row_ind) {
	uint32_t value = (uint32_t)(((uint32_t)(row[row_ind]) << 24) | ((uint32_t)(row[row_ind + 1]) << 16) |
															((uint32_t)(row[row_ind + 2]) << 8) | (uint32_t)row[row_ind + 3]);
	return value;
}

uint64_t database_uint64(uint8_t *row, uint8_t row_ind) {
	uint64_t value = (uint64_t)(((uint64_t)(row[row_ind]) << 56) | ((uint64_t)(row[row_ind + 1]) << 48) |
															((uint64_t)(row[row_ind + 2]) << 40) | ((uint64_t)(row[row_ind + 3]) << 32) |
															((uint64_t)(row[row_ind + 4]) << 24) | ((uint64_t)(row[row_ind + 5]) << 16) |
															((uint64_t)(row[row_ind + 6]) << 8) | (uint64_t)row[row_ind + 7]);
	return value;
}

int8_t database_int8(uint8_t *row, uint8_t row_ind) {
	int8_t value = (int8_t)row[row_ind];
	return value;
}

int16_t database_int16(uint8_t *row, uint8_t row_ind) {
	int16_t value = (int16_t)(((uint16_t)(row[row_ind]) << 8) | (uint16_t)row[row_ind + 1]);
	return value;
}

int32_t database_int32(uint8_t *row, uint8_t row_ind) {
	int32_t value = (int32_t)(((uint32_t)(row[row_ind]) << 24) | ((uint32_t)(row[row_ind + 1]) << 16) |
														((uint32_t)(row[row_ind + 2]) << 8) | (uint32_t)row[row_ind + 3]);
	return value;
}

int64_t database_int64(uint8_t *row, uint8_t row_ind) {
	int64_t value =
			(int64_t)(((uint64_t)(row[row_ind]) << 56) | ((uint64_t)(row[row_ind + 1]) << 48) | ((uint64_t)(row[row_ind + 2]) << 40) |
								((uint64_t)(row[row_ind + 3]) << 32) | ((uint64_t)(row[row_ind + 4]) << 24) |
								((uint64_t)(row[row_ind + 5]) << 16) | ((uint64_t)(row[row_ind + 6]) << 8) | (uint64_t)row[row_ind + 7]);
	return value;
}

uint8_t *database_blob(uint8_t *row, uint8_t row_ind) {
	uint8_t *value = &row[row_ind];
	return value;
}
