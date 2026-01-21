#include "../lib/logger.h"
#include "../lib/octet.h"
#include "device.h"
#include "user-device.h"
#include "user-zone.h"
#include "user.h"
#include "zone.h"
#include <stdio.h>

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

	if (octet_rmdir(db->directory) == -1) {
		return -1;
	}

	info("removed directory %s\n", db->directory);

	return 0;
}
