#pragma once

#include <stdint.h>

const char *human_severity(uint8_t severity);
const char *human_field(uint8_t field);
const char *human_edge(uint8_t edge);
void human_value(char (*buffer)[8], uint8_t field, int32_t value);
