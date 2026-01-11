#pragma once

#include <sqlite3.h>
#include <stdint.h>

uint16_t database_error(sqlite3 *database, int result);

uint8_t database_uint8(uint8_t *row, uint8_t row_ind);
uint16_t database_uint16(uint8_t *row, uint8_t row_ind);
uint32_t database_uint32(uint8_t *row, uint8_t row_ind);
uint64_t database_uint64(uint8_t *row, uint8_t row_ind);

int8_t database_int8(uint8_t *row, uint8_t row_ind);
int16_t database_int16(uint8_t *row, uint8_t row_ind);
int32_t database_int32(uint8_t *row, uint8_t row_ind);
int64_t database_int64(uint8_t *row, uint8_t row_ind);

uint8_t *database_blob(uint8_t *row, uint8_t row_ind);
