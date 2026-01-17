#pragma once

#include "octet.h"
#include <arpa/inet.h>
#include <sqlite3.h>

void handle(octet_t db, sqlite3 *database, char *request_buffer, char *response_buffer, int *client_sock,
						struct sockaddr_in *client_addr);
