#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <time.h>

typedef struct file_t {
	int fd;
	char *ptr;
	size_t len;
	const char *path;
	time_t modified;
	bool hydrated;
} file_t;

const char *type(const char *path);

int file(file_t *file);
