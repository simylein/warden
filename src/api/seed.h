#pragma once

#include "../lib/octet.h"
#include <sqlite3.h>

int seed(octet_t *db, sqlite3 *database);
