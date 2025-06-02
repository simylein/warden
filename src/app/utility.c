#include "utility.h"
#include "hydrate.h"
#include <stdbool.h>
#include <string.h>

void common(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const keymap_t commons[5] = {{.ptr = "font-sans",
																.len = 9,
																.val = "font-family:ui-sans-serif,system-ui,sans-serif,"
																			 "'Apple Color Emoji','Segoe UI Emoji','Segoe UI Symbol','Noto Color Emoji'"},
															 {.ptr = "font-serif",
																.len = 10,
																.val = "font-family:ui-serif,"
																			 "Georgia,Cambria,'Times New Roman',Times,serif"},
															 {.ptr = "font-mono",
																.len = 9,
																.val = "font-family:ui-monospace,"
																			 "SFMono-Regular,Menlo,Monaco,Consolas,'Liberation Mono','Courier New',monospace"},
															 {.ptr = "box-border", .len = 10, .val = "box-sizing:border-box"},
															 {.ptr = "box-content", .len = 11, .val = "box-sizing:content-box"}};

	for (uint8_t index = 0; index < sizeof(commons) / sizeof(keymap_t); index++) {
		if (cls->len == commons[index].len && memcmp(cls->ptr, commons[index].ptr, commons[index].len) == 0) {
			if (*buffer_len < sizeof(*buffer) - 64) {
				*buffer_len +=
						sprintf(&(*buffer)[*buffer_len], ".%.*s%.*s{%s}", pfx->len, pfx->ptr, cls->len, cls->ptr, commons[index].val);
			}
		}
	}
}

void position(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const class_t positions[5] = {{.ptr = "static", .len = 6},
																{.ptr = "relative", .len = 8},
																{.ptr = "absolute", .len = 8},
																{.ptr = "fixed", .len = 5},
																{.ptr = "sticky", .len = 6}};

	for (uint8_t index = 0; index < sizeof(positions) / sizeof(class_t); index++) {
		if (cls->len == positions[index].len && memcmp(cls->ptr, positions[index].ptr, positions[index].len) == 0) {
			if (*buffer_len < sizeof(*buffer) - 64) {
				*buffer_len += sprintf(&(*buffer)[*buffer_len], ".%.*s%.*s{position:%.*s}", pfx->len, pfx->ptr, cls->len, cls->ptr,
															 cls->len, cls->ptr);
			}
		}
	}
}

void display(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const class_t displays[6] = {{.ptr = "block", .len = 5}, {.ptr = "inline", .len = 6}, {.ptr = "flex", .len = 4},
															 {.ptr = "grid", .len = 4},	 {.ptr = "none", .len = 4},		{.ptr = "contents", .len = 8}};

	for (uint8_t index = 0; index < sizeof(displays) / sizeof(class_t); index++) {
		if (cls->len == displays[index].len && memcmp(cls->ptr, displays[index].ptr, displays[index].len) == 0) {
			if (*buffer_len < sizeof(*buffer) - 64) {
				*buffer_len += sprintf(&(*buffer)[*buffer_len], ".%.*s%.*s{display:%.*s}", pfx->len, pfx->ptr, cls->len, cls->ptr,
															 cls->len, cls->ptr);
			}
		}
	}
}

void spacing(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const keymap_t variants[10] = {
			{.ptr = "m-", .len = 2, .val = "margin"},					 {.ptr = "mt-", .len = 3, .val = "margin-top"},
			{.ptr = "mr-", .len = 3, .val = "margin-right"},	 {.ptr = "mb-", .len = 3, .val = "margin-bottom"},
			{.ptr = "ml-", .len = 3, .val = "margin-left"},		 {.ptr = "p-", .len = 2, .val = "padding"},
			{.ptr = "pt-", .len = 3, .val = "padding-top"},		 {.ptr = "pr-", .len = 3, .val = "padding-right"},
			{.ptr = "pb-", .len = 3, .val = "padding-bottom"}, {.ptr = "pl-", .len = 3, .val = "padding-left"}};

	const keymap_t mappings[25] = {
			{.ptr = "0", .len = 1, .val = "0px"},		 {.ptr = "1", .len = 1, .val = "4px"},		{.ptr = "2", .len = 1, .val = "8px"},
			{.ptr = "3", .len = 1, .val = "12px"},	 {.ptr = "4", .len = 1, .val = "16px"},		{.ptr = "5", .len = 1, .val = "20px"},
			{.ptr = "6", .len = 1, .val = "24px"},	 {.ptr = "7", .len = 1, .val = "28px"},		{.ptr = "8", .len = 1, .val = "32px"},
			{.ptr = "9", .len = 1, .val = "36px"},	 {.ptr = "10", .len = 2, .val = "40px"},	{.ptr = "11", .len = 2, .val = "44px"},
			{.ptr = "12", .len = 2, .val = "48px"},	 {.ptr = "13", .len = 2, .val = "52px"},	{.ptr = "14", .len = 2, .val = "56px"},
			{.ptr = "15", .len = 2, .val = "60px"},	 {.ptr = "16", .len = 2, .val = "64px"},	{.ptr = "20", .len = 2, .val = "80px"},
			{.ptr = "24", .len = 2, .val = "96px"},	 {.ptr = "28", .len = 2, .val = "112px"}, {.ptr = "32", .len = 2, .val = "128px"},
			{.ptr = "48", .len = 2, .val = "192px"}, {.ptr = "64", .len = 2, .val = "256px"}, {.ptr = "80", .len = 2, .val = "320px"},
			{.ptr = "96", .len = 2, .val = "384px"}};

	for (uint8_t index = 0; index < sizeof(variants) / sizeof(keymap_t); index++) {
		if (cls->len > variants[index].len && memcmp(cls->ptr, variants[index].ptr, variants[index].len) == 0) {
			const keymap_t *mapping = NULL;
			for (uint8_t ind = 0; ind < sizeof(mappings) / sizeof(keymap_t); ind++) {
				if (cls->len - variants[index].len == mappings[ind].len &&
						memcmp(&cls->ptr[variants[index].len], mappings[ind].ptr, mappings[ind].len) == 0) {
					mapping = &mappings[ind];
					break;
				}
			}
			if (mapping == NULL) {
				return;
			}

			if (*buffer_len < sizeof(*buffer) - 64) {
				(*buffer)[*buffer_len] = '.';
				*buffer_len += 1;
				if (pfx->len > 0) {
					*buffer_len += sprintf(&(*buffer)[*buffer_len], "%.*s%s", pfx->len, pfx->ptr, "\\:");
				}
				*buffer_len += sprintf(&(*buffer)[*buffer_len], "%.*s{%s:%s}", cls->len, cls->ptr, variants[index].val, mapping->val);
			}
		}
	}
}

void sizing(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const keymap_t variants[6] = {
			{.ptr = "w-", .len = 2, .val = "width"},					{.ptr = "min-w-", .len = 6, .val = "min-width"},
			{.ptr = "max-w-", .len = 6, .val = "max-width"},	{.ptr = "h-", .len = 2, .val = "height"},
			{.ptr = "min-h-", .len = 6, .val = "min-height"}, {.ptr = "max-h-", .len = 6, .val = "max-height"}};

	const keymap_t mappings[30] = {{.ptr = "0", .len = 1, .val = "0px"},			{.ptr = "1", .len = 1, .val = "4px"},
																 {.ptr = "2", .len = 1, .val = "8px"},			{.ptr = "3", .len = 1, .val = "12px"},
																 {.ptr = "4", .len = 1, .val = "16px"},			{.ptr = "5", .len = 1, .val = "20px"},
																 {.ptr = "6", .len = 1, .val = "24px"},			{.ptr = "7", .len = 1, .val = "28px"},
																 {.ptr = "8", .len = 1, .val = "32px"},			{.ptr = "9", .len = 1, .val = "36px"},
																 {.ptr = "10", .len = 2, .val = "40px"},		{.ptr = "11", .len = 2, .val = "44px"},
																 {.ptr = "12", .len = 2, .val = "48px"},		{.ptr = "13", .len = 2, .val = "52px"},
																 {.ptr = "14", .len = 2, .val = "56px"},		{.ptr = "15", .len = 2, .val = "60px"},
																 {.ptr = "16", .len = 2, .val = "64px"},		{.ptr = "20", .len = 2, .val = "80px"},
																 {.ptr = "24", .len = 2, .val = "96px"},		{.ptr = "28", .len = 2, .val = "112px"},
																 {.ptr = "32", .len = 2, .val = "128px"},		{.ptr = "48", .len = 2, .val = "192px"},
																 {.ptr = "64", .len = 2, .val = "256px"},		{.ptr = "80", .len = 2, .val = "320px"},
																 {.ptr = "96", .len = 2, .val = "384px"},		{.ptr = "full", .len = 4, .val = "100%"},
																 {.ptr = "vh", .len = 2, .val = "100vh"},		{.ptr = "svh", .len = 3, .val = "100svh"},
																 {.ptr = "lvh", .len = 3, .val = "100lvh"}, {.ptr = "dvh", .len = 3, .val = "100dvh"}};

	for (uint8_t index = 0; index < sizeof(variants) / sizeof(keymap_t); index++) {
		if (cls->len > variants[index].len && memcmp(cls->ptr, variants[index].ptr, variants[index].len) == 0) {
			const keymap_t *mapping = NULL;
			for (uint8_t ind = 0; ind < sizeof(mappings) / sizeof(keymap_t); ind++) {
				if (cls->len - variants[index].len == mappings[ind].len &&
						memcmp(&cls->ptr[variants[index].len], mappings[ind].ptr, mappings[ind].len) == 0) {
					mapping = &mappings[ind];
					break;
				}
			}
			if (mapping == NULL) {
				return;
			}

			if (*buffer_len < sizeof(*buffer) - 64) {
				(*buffer)[*buffer_len] = '.';
				*buffer_len += 1;
				if (pfx->len > 0) {
					*buffer_len += sprintf(&(*buffer)[*buffer_len], "%.*s%s", pfx->len, pfx->ptr, "\\:");
				}
				*buffer_len += sprintf(&(*buffer)[*buffer_len], "%.*s{%s:%s}", cls->len, cls->ptr, variants[index].val, mapping->val);
			}
		}
	}
}

void text(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const keymap_t texts[8] = {
			{.ptr = "text-xs", .len = 7, .val = "font-size:12px"},	 {.ptr = "text-sm", .len = 7, .val = "font-size:14px"},
			{.ptr = "text-base", .len = 9, .val = "font-size:16px"}, {.ptr = "text-lg", .len = 7, .val = "font-size:18px"},
			{.ptr = "text-xl", .len = 7, .val = "font-size:20px"},	 {.ptr = "text-2xl", .len = 8, .val = "font-size:24px"},
			{.ptr = "text-3xl", .len = 8, .val = "font-size:30px"},	 {.ptr = "text-4xl", .len = 8, .val = "font-size:36px"},
	};

	for (uint8_t index = 0; index < sizeof(texts) / sizeof(keymap_t); index++) {
		if (cls->len == texts[index].len && memcmp(cls->ptr, texts[index].ptr, texts[index].len) == 0) {
			if (*buffer_len < sizeof(*buffer) - 64) {
				*buffer_len +=
						sprintf(&(*buffer)[*buffer_len], ".%.*s%.*s{%s}", pfx->len, pfx->ptr, cls->len, cls->ptr, texts[index].val);
			}
		}
	}
}

void font(class_t *pfx, class_t *cls, char (*buffer)[2048], uint16_t *buffer_len, breakpoint_t *breakpoint) {
	if (pfx->len != breakpoint->tag_len || memcmp(pfx->ptr, breakpoint->tag, breakpoint->tag_len) != 0) {
		return;
	}

	const keymap_t fonts[9] = {
			{.ptr = "font-thin", .len = 9, .val = "font-weight:100"},
			{.ptr = "font-extralight", .len = 15, .val = "font-weight:200"},
			{.ptr = "font-light", .len = 10, .val = "font-weight:300"},
			{.ptr = "font-normal", .len = 11, .val = "font-weight:400"},
			{.ptr = "font-medium", .len = 11, .val = "font-weight:500"},
			{.ptr = "font-semibold", .len = 13, .val = "font-weight:600"},
			{.ptr = "font-bold", .len = 10, .val = "font-weight:700"},
			{.ptr = "font-extrabold", .len = 14, .val = "font-weight:800"},
			{.ptr = "font-black", .len = 11, .val = "font-weight:900"},
	};

	for (uint8_t index = 0; index < sizeof(fonts) / sizeof(keymap_t); index++) {
		if (cls->len == fonts[index].len && memcmp(cls->ptr, fonts[index].ptr, fonts[index].len) == 0) {
			if (*buffer_len < sizeof(*buffer) - 64) {
				*buffer_len +=
						sprintf(&(*buffer)[*buffer_len], ".%.*s%.*s{%s}", pfx->len, pfx->ptr, cls->len, cls->ptr, fonts[index].val);
			}
		}
	}
}
