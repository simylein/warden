#include "email.h"
#include "../lib/logger.h"
#include "../lib/octet.h"
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *email_file = "email";

const email_row_t email_row = {
		.address_len = 0,
		.address = 1,
		.port = 17,
		.from_len = 19,
		.from = 20,
		.to_len = 52,
		.to = 53,
		.size = 85,
};

uint16_t email_select_one(octet_t *db, email_t *email) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, email_file) == -1) {
		error("failed to sprintf file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDONLY, F_RDLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("select email\n");

	if (stmt.stat.st_size == 0) {
		warn("no email for sending configured\n");
		status = 404;
		goto cleanup;
	}

	off_t offset = 0;
	if (octet_row_read(&stmt, file, offset, db->row, email_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	uint8_t address_len = octet_uint8_read(db->row, email_row.address_len);
	char *address = octet_text_read(db->row, email_row.address);
	uint16_t port = octet_uint16_read(db->row, email_row.port);
	uint8_t from_len = octet_uint8_read(db->row, email_row.from_len);
	char *from = octet_text_read(db->row, email_row.from);
	uint8_t to_len = octet_uint8_read(db->row, email_row.to_len);
	char *to = octet_text_read(db->row, email_row.to);
	memcpy(email->address, address, address_len);
	email->address_len = address_len;
	email->port = port;
	memcpy(email->from, from, from_len);
	email->from_len = from_len;
	memcpy(email->to, to, to_len);
	email->to_len = to_len;

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}

uint16_t email_insert(octet_t *db, email_t *email) {
	uint16_t status;

	char file[128];
	if (sprintf(file, "%s/%s.data", db->directory, email_file) == -1) {
		error("failed to sprintf file\n");
		return 500;
	}

	octet_stmt_t stmt;
	if (octet_open(&stmt, file, O_RDWR, F_WRLCK) == -1) {
		status = octet_error();
		goto cleanup;
	}

	debug("insert email %.*s:%hu\n", email->address_len, email->address, email->port);

	off_t offset = 0;
	octet_uint8_write(db->row, email_row.address_len, email->address_len);
	octet_text_write(db->row, email_row.address, email->address, email->address_len);
	octet_uint16_write(db->row, email_row.port, email->port);
	octet_uint8_write(db->row, email_row.from_len, email->from_len);
	octet_text_write(db->row, email_row.from, email->from, email->from_len);
	octet_uint8_write(db->row, email_row.to_len, email->to_len);
	octet_text_write(db->row, email_row.to, email->to, email->to_len);

	if (octet_row_write(&stmt, file, offset, db->row, email_row.size) == -1) {
		status = octet_error();
		goto cleanup;
	}

	status = 0;

cleanup:
	octet_close(&stmt, file);
	return status;
}
