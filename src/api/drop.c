#include "../lib/error.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "downlink.h"
#include "host.h"
#include "metric.h"
#include "radio.h"
#include "reading.h"
#include "uplink.h"
#include "user-device.h"
#include "user-zone.h"
#include "user.h"
#include "zone.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

int drop_user(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	if (octet_unlink(file) == -1) {
		return -1;
	}

	info("unlinked file %s\n", user_file);
	return 0;
}

int drop_device(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	if (octet_unlink(file) == -1) {
		return -1;
	}

	info("unlinked file %s\n", device_file);
	return 0;
}

int drop_user_device(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	if (octet_unlink(file) == -1) {
		return -1;
	}

	info("unlinked file %s\n", user_device_file);
	return 0;
}

int drop_zone(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	if (octet_unlink(file) == -1) {
		return -1;
	}

	info("unlinked file %s\n", zone_file);
	return 0;
}

int drop_user_zone(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	if (octet_unlink(file) == -1) {
		return -1;
	}

	info("unlinked file %s\n", user_zone_file);
	return 0;
}

int drop_host(octet_t *db) {
	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, host_file) == -1) {
		error("failed to sprintf to file\n");
		return -1;
	}

	if (octet_unlink(file) == -1) {
		return -1;
	}

	info("unlinked file %s\n", host_file);
	return 0;
}

int drop(octet_t *db) {
	if (drop_user(db) == -1) {
		return -1;
	}
	if (drop_device(db) == -1) {
		return -1;
	}
	if (drop_user_device(db) == -1) {
		return -1;
	}
	if (drop_zone(db) == -1) {
		return -1;
	}
	if (drop_user_zone(db) == -1) {
		return -1;
	}
	if (drop_host(db) == -1) {
		return -1;
	}

	DIR *db_directory = opendir(db->directory);
	if (db_directory == NULL) {
		error("failed to open %s because %s\n", db->directory, errno_str());
		return -1;
	}

	struct dirent *dir;
	while ((dir = readdir(db_directory)) != NULL) {
		if (dir->d_type == DT_DIR && strcmp(dir->d_name, ".") != 0 && strcmp(dir->d_name, "..") != 0) {
			const char *files[] = {uplink_file, downlink_file, reading_file, metric_file, buffer_file, config_file, radio_file};
			for (uint8_t index = 0; index < sizeof(files) / sizeof(files[0]); index++) {
				char file[512];
				if (sprintf(file, "%s/%s/%s.data", db->directory, dir->d_name, files[index]) == -1) {
					error("failed to sprintf uuid to file\n");
					return -1;
				}

				if (octet_unlink(file) == -1) {
					return -1;
				}
			}

			char directory[512];
			if (sprintf(directory, "%s/%s", db->directory, dir->d_name) == -1) {
				error("failed to sprintf to file\n");
				return -1;
			}

			if (octet_rmdir(directory) == -1) {
				return -1;
			}
		}
	}

	if (closedir(db_directory) == -1) {
		error("failed to close %s because %s\n", db->directory, errno_str());
		return -1;
	}

	if (octet_rmdir(db->directory) == -1) {
		return -1;
	}

	info("removed directory %s\n", db->directory);

	return 0;
}
