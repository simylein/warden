#pragma once

#include <stdint.h>
#include <sys/stat.h>
#include <unistd.h>

typedef struct octet_stmt_t {
	int fd;
	struct stat stat;
} octet_stmt_t;

int octet_open(octet_stmt_t *stmt, const char *file, int open_flags, short lock_type);
void octet_close(octet_stmt_t *stmt, const char *file);

ssize_t octet_row_read(octet_stmt_t *stmt, const char *file, off_t offset, uint8_t *row, uint8_t row_size);
ssize_t octet_row_write(octet_stmt_t *stmt, const char *file, off_t offset, uint8_t *row, uint8_t row_size);

uint8_t octet_uint8_read(uint8_t *row, uint8_t row_ind);
uint16_t octet_uint16_read(uint8_t *row, uint8_t row_ind);
uint32_t octet_uint32_read(uint8_t *row, uint8_t row_ind);
uint64_t octet_uint64_read(uint8_t *row, uint8_t row_ind);

int8_t octet_int8_read(uint8_t *row, uint8_t row_ind);
int16_t octet_int16_read(uint8_t *row, uint8_t row_ind);
int32_t octet_int32_read(uint8_t *row, uint8_t row_ind);
int64_t octet_int64_read(uint8_t *row, uint8_t row_ind);

uint8_t *octet_blob_read(uint8_t *row, uint8_t row_ind);

void octet_uint8_write(uint8_t *row, uint8_t row_ind, uint8_t value);
void octet_uint16_write(uint8_t *row, uint8_t row_ind, uint16_t value);
void octet_uint32_write(uint8_t *row, uint8_t row_ind, uint32_t value);
void octet_uint64_write(uint8_t *row, uint8_t row_ind, uint64_t value);

void octet_int8_write(uint8_t *row, uint8_t row_ind, int8_t value);
void octet_int16_write(uint8_t *row, uint8_t row_ind, int16_t value);
void octet_int32_write(uint8_t *row, uint8_t row_ind, int32_t value);
void octet_int64_write(uint8_t *row, uint8_t row_ind, int64_t value);

void octet_blob_write(uint8_t *row, uint8_t row_ind, uint8_t *value, uint8_t value_len);
