#pragma once

#include <stdio.h>

typedef struct file_t {
	int fd;
	char *ptr;
	size_t len;
	const char *name;
} file_t;

const char *type(const char *file_name);

int file(const char *path, file_t *file);
