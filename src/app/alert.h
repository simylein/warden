#pragma once

#include <pthread.h>
#include "../lib/octet.h"


extern pthread_t alerter_thread;
extern octet_t alerter_octet;
extern char *alerter_buffer;

void *alerter(void *args);
