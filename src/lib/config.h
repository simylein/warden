#pragma once

#include <stdbool.h>
#include <stdint.h>

extern const char *name;

extern const char *address;
extern uint16_t port;

extern const char *database_file;
extern uint16_t database_timeout;

extern uint8_t log_level;
extern bool log_requests;
extern bool log_responses;

int configure(int argc, char *argv[]);

const char *human_bool(bool val);
const char *human_log_level(uint8_t level);
