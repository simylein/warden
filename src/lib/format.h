#pragma once

#include <stdio.h>
#include <time.h>

void human_bytes(char (*buffer)[8], size_t bytes);
void human_duration(char (*buffer)[8], struct timespec *start, struct timespec *stop);
