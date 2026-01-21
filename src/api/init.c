#include "../lib/error.h"
#include "../lib/logger.h"
#include "device.h"
#include "user-device.h"
#include "user-zone.h"
#include "user.h"
#include "zone.h"
#include <fcntl.h>
#include <stdio.h>

int init_user(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	int fd = open(file, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		error("failed to create file %s because %s\n", file, errno_str());
		return -1;
	}

	info("created file %s\n", user_file);

	close(fd);
	return 0;
}

int init_device(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	int fd = open(file, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		error("failed to create file %s because %s\n", file, errno_str());
		return -1;
	}

	info("created file %s\n", device_file);

	close(fd);
	return 0;
}

int init_user_device(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	int fd = open(file, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		error("failed to create file %s because %s\n", file, errno_str());
		return -1;
	}

	info("created file %s\n", user_device_file);

	close(fd);
	return 0;
}

int init_zone(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	int fd = open(file, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		error("failed to create file %s because %s\n", file, errno_str());
		return -1;
	}

	info("created file %s\n", zone_file);

	close(fd);
	return 0;
}

int init_user_zone(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	int fd = open(file, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		error("failed to create file %s because %s\n", file, errno_str());
		return -1;
	}

	info("created file %s\n", user_zone_file);

	close(fd);
	return 0;
}

int init(octet_t *db) {
	if (mkdir(db->directory, S_IRUSR | S_IWUSR | S_IXUSR) == -1) {
		error("failed to create directory %s because %s\n", db->directory, errno_str());
		return -1;
	}

	info("created directory %s\n", db->directory);

	if (init_user(db) == -1) {
		return -1;
	}
	if (init_device(db) == -1) {
		return -1;
	}
	if (init_user_device(db) == -1) {
		return -1;
	}
	if (init_zone(db) == -1) {
		return -1;
	}
	if (init_user_zone(db) == -1) {
		return -1;
	}

	return 0;
}
