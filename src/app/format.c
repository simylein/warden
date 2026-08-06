#include "../lib/format.h"
#include <stdint.h>
#include <stdio.h>

const char *human_severity(uint8_t severity) {
	switch (severity) {
	case 0:
		return "critical";
	case 1:
		return "warning";
	case 2:
		return "info";
	default:
		return "???";
	}
}

const char *human_field(uint8_t field) {
	switch (field) {
	case 0:
		return "temperature";
	case 1:
		return "humidity";
	case 2:
		return "dewpoint";
	case 3:
		return "photovoltaic";
	case 4:
		return "battery";
	case 5:
		return "delay";
	case 6:
		return "level";
	case 7:
		return "receive delay";
	case 8:
		return "send delay";
	default:
		return "???";
	}
}

const char *human_edge(uint8_t edge) {
	switch (edge) {
	case 0:
		return "low";
	case 1:
		return "high";
	default:
		return "???";
	}
}

void human_value(char (*buffer)[8], uint8_t field, int32_t value) {
	switch (field) {
	case 0:
		sprintf(*buffer, "%0.2f", (float)value / 100.0f);
		break;
	case 1:
		sprintf(*buffer, "%0.2f", (float)value / 100.0f);
		break;
	case 2:
		sprintf(*buffer, "%0.2f", (float)value / 100.0f);
		break;
	case 3:
		sprintf(*buffer, "%0.3f", (float)value / 1000.0f);
		break;
	case 4:
		sprintf(*buffer, "%0.3f", (float)value / 1000.0f);
		break;
	case 5:
		human_time(buffer, (time_t)value);
		break;
	case 6:
		sprintf(*buffer, "%d", value);
		break;
	case 7:
		human_time(buffer, (time_t)value);
		break;
	case 8:
		human_time(buffer, (time_t)value);
		break;
	default:
		sprintf(*buffer, "%d", value);
		break;
	}
}
