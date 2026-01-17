#pragma once

#include "../lib/octet.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>

void route(octet_t *db, sqlite3 *database, request_t *request, response_t *response);
