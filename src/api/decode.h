#pragma once

#include "../lib/octet.h"
#include "uplink.h"
#include <sqlite3.h>
#include <stdint.h>

uint16_t decode(octet_t *db, sqlite3 *database, uplink_t *uplink);
