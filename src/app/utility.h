#pragma once

#include "hydrate.h"

typedef struct keymap_t {
	char *ptr;
	uint8_t len;
	const char *val;
} keymap_t;

void common(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint);
void position(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint);
void display(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint);
void spacing(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint);
void sizing(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint);
