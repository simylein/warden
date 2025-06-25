#pragma once

#include "../lib/bwt.h"
#include "../lib/request.h"
#include "../lib/response.h"
#include <sqlite3.h>

void recap_find(sqlite3 *database, bwt_t *bwt, request_t *request, response_t *response);
