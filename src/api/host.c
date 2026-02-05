#include "host.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *host_file = "host";

const host_row_t host_row = {
		.id = 0,
		.address_len = 16,
		.address = 17,
		.port = 49,
		.username_len = 51,
		.username = 52,
		.password_len = 68,
		.password = 69,
		.size = 133,
};

uint16_t host_select_one(octet_t *db, host_t *host) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, host_file) == -1) {
		error("failed to sprintf file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select host\n");

	if (stmt.stat.st_size == 0) {
		warn("no host for schedules configured\n");
		status = 404;
		goto cleanup;
	}

	off_t offset = 0;
	if (octet_row_read(&stmt, file, offset, db->row, host_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	uint8_t address_len = octet_uint8_read(db->row, host_row.address_len);
	char *address = octet_text_read(db->row, host_row.address);
	uint16_t port = octet_uint16_read(db->row, host_row.port);
	uint8_t username_len = octet_uint8_read(db->row, host_row.username_len);
	char *username = octet_text_read(db->row, host_row.username);
	uint8_t password_len = octet_uint8_read(db->row, host_row.password_len);
	char *password = octet_text_read(db->row, host_row.password);
	memcpy(host->address, address, address_len);
	host->address_len = address_len;
	host->port = port;
	memcpy(host->username, username, username_len);
	host->username_len = username_len;
	memcpy(host->password, password, password_len);
	host->password_len = password_len;

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}
