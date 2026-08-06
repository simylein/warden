#pragma once

#include "../api/alert.h"
#include "../api/device.h"
#include "../lib/octet.h"

int email_send(octet_t *db, alert_t *alert, device_t *device);
