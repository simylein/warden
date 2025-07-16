#pragma once

#include <stdint.h>
#include <stdio.h>

int strnto16(const char *string, const size_t string_len, uint16_t *value);
int strnto32(const char *string, const size_t string_len, uint32_t *value);
int strnto64(const char *string, const size_t string_len, uint64_t *value);
