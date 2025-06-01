#pragma once

#include "../lib/response.h"
#include "file.h"

extern file_t signin;
extern file_t signup;

void serve(file_t *asset, response_t *response);
