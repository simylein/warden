#pragma once

#include "../lib/octet.h"
#include "uplink.h"
#include <stdint.h>

uint16_t decode(octet_t *db, uplink_t *uplink);
