#pragma once

#include "uplink.h"
#include <sqlite3.h>

int decode(sqlite3 *database, uplink_t *uplink);
