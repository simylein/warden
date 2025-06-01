#pragma once

#include <stdio.h>

typedef struct file_t {
	int fd;
	char *ptr;
	size_t len;
	const char *path;
} file_t;

const char *type(const char *path);

int file(file_t *file);
