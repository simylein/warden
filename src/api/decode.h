#pragma once

#include "uplink.h"
#include <sqlite3.h>
#include <stdint.h>

uint16_t decode(sqlite3 *database, uplink_t *uplink);
