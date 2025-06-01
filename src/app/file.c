#include "file.h"
#include "../lib/error.h"
#include "../lib/logger.h"
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

const char *type(const char *path) {
	const char *extension = strrchr(path, '.');
	if (extension == NULL) {
		error("file path %s has no extension\n", path);
		return "text/unknown";
	}
	if (strcmp(extension, ".txt") == 0) {
		return "text/plain";
	}
	if (strcmp(extension, ".html") == 0) {
		return "text/html";
	}
	error("unknown content type %s\n", extension);
	return "unknown";
}

int file(file_t *file) {
	if (file->fd == -1) {
		trace("opening file %s\n", file->path);

		file->fd = open(file->path, O_RDONLY);
		if (file->fd == -1) {
			error("failed to open %s because %s\n", file->path, errno_str());
			return -1;
		}
	}

	struct stat file_stat;
	if (fstat(file->fd, &file_stat) == -1) {
		error("failed to stat %s because %s\n", file->path, errno_str());
		return -1;
	}

	if (file->ptr == NULL) {
		debug("reading file %s\n", file->path);

		file->ptr = realloc(file->ptr, (size_t)file_stat.st_size);
		if (file->ptr == NULL) {
			error("failed to allocate %zu bytes for %s because %s\n", (size_t)file_stat.st_size, file->path, errno_str());
			return -1;
		}

		if (lseek(file->fd, 0, SEEK_SET) == -1) {
			error("failed to seek to start of %s because %s\n", file->path, errno_str());
			return -1;
		}

		ssize_t bytes_read = read(file->fd, file->ptr, (size_t)file_stat.st_size);
		if (bytes_read != file_stat.st_size) {
			error("failed to read %zu bytes from %s because %s\n", (size_t)(file_stat.st_size - bytes_read), file->path, errno_str());
			return -1;
		}

		file->len = (size_t)file_stat.st_size;
	}

	return 0;
}
