#pragma once

#include <stdint.h>
#include <stdio.h>

int strnfind(const char *buffer, const size_t buffer_len, const char *prefix, const char *suffix, const char **string,
						 size_t *string_len, size_t string_len_max);

const char *strncasestrn(const char *buffer, size_t buffer_len, const char *buf, size_t buf_len);

int strnto16(const char *string, const size_t string_len, uint16_t *value);
int strnto32(const char *string, const size_t string_len, uint32_t *value);
int strnto64(const char *string, const size_t string_len, uint64_t *value);
