#pragma once

#include "uplink.h"
#include <sqlite3.h>
#include <stdint.h>

uint16_t decode(const char *db, sqlite3 *database, uplink_t *uplink);
