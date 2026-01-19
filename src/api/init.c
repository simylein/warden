#include "../lib/logger.h"
#include "../lib/octet.h"
#include "device.h"
#include "user-device.h"
#include "user-zone.h"
#include "user.h"
#include "zone.h"
#include <fcntl.h>
#include <stdio.h>

int init_user(octet_t *db) {
	int status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_CREAT | O_EXCL, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	info("created file %s\n", user_file);
	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

int init_device(octet_t *db) {
	int status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_CREAT | O_EXCL, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	info("created file %s\n", device_file);
	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

int init_user_device(octet_t *db) {
	int status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_device_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_CREAT | O_EXCL, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	info("created file %s\n", user_device_file);
	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

int init_zone(octet_t *db) {
	int status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_CREAT | O_EXCL, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	info("created file %s\n", zone_file);
	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

int init_user_zone(octet_t *db) {
	int status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, user_zone_file) == -1) {
		error("failed to sprintf to file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_CREAT | O_EXCL, F_RDLCK) == -1) {
		status = -1;
		goto cleanup;
	}

	info("created file %s\n", user_zone_file);
	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

int init(octet_t *db) {
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
