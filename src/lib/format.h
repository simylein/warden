#pragma once

#include <stdio.h>
#include <time.h>

const char *human_bool(bool val);
const char *human_log_level(uint8_t level);

void human_bytes(char (*buffer)[8], size_t bytes);
void human_duration(char (*buffer)[8], struct timespec *start, struct timespec *stop);
