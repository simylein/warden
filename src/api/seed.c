#include "../lib/logger.h"
#include "../lib/octet.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "downlink.h"
#include "host.h"
#include "metric.h"
#include "radio.h"
#include "reading.h"
#include "uplink.h"
#include "user-device.h"
#include "user-zone.h"
#include "user.h"
#include "zone.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint8_t (*user_ids)[8];
uint8_t user_ids_len;
uint8_t (*zone_ids)[8];
uint8_t zone_ids_len;
uint8_t (*device_ids)[8];
uint8_t device_ids_len;

int seed_user(octet_t *db) {
	char *usernames[] = {"alice", "bob", "charlie", "dave"};
	char *passwords[] = {".go4Alice", ".go4Bob", ".go4Charlie", ".go4Dave"};

	user_ids_len = sizeof(usernames) / sizeof(*usernames);
	user_ids = malloc(user_ids_len * sizeof(*user_ids));
	if (user_ids == NULL) {
		return -1;
	}

	for (uint8_t index = 0; index < user_ids_len; index++) {
		uint8_t permissions[8];
		user_t user = {
				.id = &user_ids[index],
				.username = usernames[index],
				.username_len = (uint8_t)strlen(usernames[index]),
				.password = passwords[index],
				.password_len = (uint8_t)strlen(passwords[index]),
				.permissions = &permissions,
				.signup_at = (time_t[]){time(NULL)},
				.signin_at = (time_t[]){time(NULL)},
		};

		if (user_insert(db, &user) != 0) {
			return -1;
		}
	}

	info("seeded file %s\n", user_file);
	return 0;
}

int seed_zone(octet_t *db) {
	char *names[] = {"indoor", "outdoor"};
	uint8_t colors[][12] = {
			{0x16, 0xa3, 0x4a, 0x4a, 0xde, 0x80, 0xf0, 0xfd, 0xf4, 0x05, 0x2e, 0x16},
			{0x02, 0x84, 0xc7, 0x38, 0xbd, 0xf8, 0xf0, 0xf9, 0xff, 0x08, 0x2f, 0x49},
	};

	zone_ids_len = sizeof(names) / sizeof(*names);
	zone_ids = malloc(zone_ids_len * sizeof(*zone_ids));
	if (zone_ids == NULL) {
		return -1;
	}

	for (uint8_t index = 0; index < zone_ids_len; index++) {
		zone_t zone = {
				.id = &zone_ids[index],
				.name = names[index],
				.name_len = (uint8_t)strlen(names[index]),
				.color = &colors[index],
				.created_at = (time_t[]){time(NULL)},
		};

		if (zone_insert(db, &zone) != 0) {
			return -1;
		}
	}

	info("seeded file %s\n", zone_file);
	return 0;
}

int seed_user_zone(octet_t *db) {
	for (uint8_t index = 0; index < user_ids_len; index++) {
		for (uint8_t ind = 0; ind < zone_ids_len; ind++) {
			if (rand() % 2 == 0) {
				user_zone_t user_zone = {
						.user_id = &user_ids[index],
						.zone_id = &zone_ids[ind],
				};

				if (user_zone_insert(db, &user_zone) != 0) {
					return -1;
				}
			}
		}
	}

	info("seeded file %s\n", user_zone_file);
	return 0;
}

int seed_device(octet_t *db) {
	device_ids_len = (uint8_t)(8 + rand() % 16);
	device_ids = malloc(device_ids_len * sizeof(*device_ids));
	if (device_ids == NULL) {
		return -1;
	}

	char *zone_names[] = {"indoor", "outdoor"};
	uint8_t zone_colors[][12] = {
			{0x16, 0xa3, 0x4a, 0x4a, 0xde, 0x80, 0xf0, 0xfd, 0xf4, 0x05, 0x2e, 0x16},
			{0x02, 0x84, 0xc7, 0x38, 0xbd, 0xf8, 0xf0, 0xf9, 0xff, 0x08, 0x2f, 0x49},
	};

	for (uint8_t index = 0; index < device_ids_len; index++) {
		char name[16];
		uint8_t name_len = (uint8_t)(4 + rand() % 12);
		for (uint8_t ind = 0; ind < name_len; ind++) {
			name[ind] = (char)('a' + rand() % 26);
		}

		uint8_t (*zone_id)[8];
		char *zone_name;
		uint8_t (*zone_color)[12];
		if (rand() % 5 == 0) {
			zone_id = NULL;
			zone_name = NULL;
			zone_color = NULL;
		} else {
			uint8_t ind = (uint8_t)(rand() % zone_ids_len);
			zone_id = &zone_ids[ind];
			zone_name = zone_names[ind];
			zone_color = &zone_colors[ind];
		}

		char firmware[16];
		uint8_t firmware_len = 0;
		if (rand() % 4 != 0) {
			firmware_len =
					(uint8_t)sprintf(firmware, "v%hhu.%hhu.%hhu", (uint8_t)(rand() % 4), (uint8_t)(rand() % 8), (uint8_t)(rand() % 16));
		}

		char hardware[16];
		uint8_t hardware_len = 0;
		if (rand() % 4 != 0) {
			hardware_len =
					(uint8_t)sprintf(hardware, "v%hhu.%hhu.%hhu", (uint8_t)(rand() % 2), (uint8_t)(rand() % 4), (uint8_t)(rand() % 8));
		}

		device_t device = {
				.id = &device_ids[index],
				.name = name,
				.name_len = name_len,
				.zone_id = zone_id,
				.zone_name = zone_name,
				.zone_name_len = zone_name == NULL ? 0 : (uint8_t)strlen(zone_name),
				.zone_color = zone_color,
				.firmware = firmware_len == 0 ? NULL : firmware,
				.firmware_len = firmware_len,
				.hardware = hardware_len == 0 ? NULL : hardware,
				.hardware_len = hardware_len,
				.created_at = (time_t[]){time(NULL)},
		};

		if (device_insert(db, &device) != 0) {
			return -1;
		}
	}

	info("seeded file %s\n", device_file);
	return 0;
}

int seed_user_device(octet_t *db) {
	for (uint8_t index = 0; index < user_ids_len; index++) {
		for (uint8_t ind = 0; ind < device_ids_len; ind++) {
			if (rand() % 2 == 0) {
				user_device_t user_device = {
						.user_id = &user_ids[index],
						.device_id = &device_ids[ind],
				};

				if (user_device_insert(db, &user_device) != 0) {
					return -1;
				}
			}
		}
	}

	info("seeded file %s\n", user_device_file);
	return 0;
}

int seed_host(octet_t *db) {
	char *address = "0.0.0.0";
	char *username = "warden";
	char *password = ".go4Warden";

	uint8_t id[8];
	host_t host = {
			.id = &id,
			.address = address,
			.address_len = (uint8_t)strlen(address),
			.port = 1278,
			.username = username,
			.username_len = (uint8_t)strlen(username),
			.password = password,
			.password_len = (uint8_t)strlen(password),
	};

	if (host_insert(db, &host) != 0) {
		return -1;
	}

	info("seeded file %s\n", host_file);
	return 0;
}

int seed_reading(octet_t *db) {
	for (uint8_t index = 0; index < device_ids_len; index++) {
		float temperature = (float)(rand() % 6000) / 100 - 20;
		float humidity = (float)(rand() % 10000) / 100;
		time_t now = time(NULL);
		time_t captured_at = time(NULL) - 2 * 24 * 60 * 60;
		while (captured_at < now) {
			reading_t reading = {
					.temperature = temperature,
					.humidity = humidity,
					.captured_at = captured_at,
					.device_id = &device_ids[index],
			};

			if (reading_insert(db, &reading) != 0) {
				return -1;
			}
			temperature += ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
			if (temperature < -20) {
				temperature += 1;
			}
			if (temperature > 40) {
				temperature -= 1;
			}
			humidity += ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
			if (humidity < 0) {
				humidity += 1;
			}
			if (humidity > 100) {
				humidity -= 1;
			}
			captured_at += 56 + rand() % 8;
		}
	}

	info("seeded file %s\n", reading_file);
	return 0;
}

int seed_metric(octet_t *db) {
	for (uint8_t index = 0; index < device_ids_len; index++) {
		float photovoltaic = (float)(rand() % 5000) / 1000;
		float battery = (float)(rand() % 1000) / 1000 + 3.2f;
		time_t now = time(NULL);
		time_t captured_at = time(NULL) - 2 * 24 * 60 * 60;
		while (captured_at < now) {
			metric_t metric = {
					.photovoltaic = photovoltaic,
					.battery = battery,
					.captured_at = captured_at,
					.device_id = &device_ids[index],
			};

			if (metric_insert(db, &metric) != 0) {
				return -1;
			}
			photovoltaic += ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
			if (photovoltaic < 0) {
				photovoltaic += 0.1f;
			}
			if (photovoltaic > 5) {
				photovoltaic -= 0.1f;
			}
			battery += ((float)rand() / (float)RAND_MAX) * 0.2f - 0.1f;
			if (battery < 3.2) {
				battery += 0.1f;
			}
			if (battery > 4.2) {
				battery -= 0.1f;
			}
			captured_at += 56 + rand() % 8;
		}
	}

	info("seeded file %s\n", metric_file);
	return 0;
}

int seed_buffer(octet_t *db) {
	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint32_t delay = 0;
		uint16_t level = 0;
		time_t now = time(NULL);
		time_t captured_at = time(NULL) - 2 * 24 * 60 * 60;
		while (captured_at < now) {
			buffer_t buffer = {
					.delay = delay,
					.level = level,
					.captured_at = captured_at,
					.device_id = &device_ids[index],
			};

			if (buffer_insert(db, &buffer) != 0) {
				return -1;
			}
			bool increase = rand() % 128 == 0;
			bool decrease = rand() % 8 != 0;
			if (increase) {
				uint16_t value = (uint16_t)(rand() % 3600);
				delay += (uint32_t)value;
				level += (uint16_t)value / 60;
			}
			if (decrease) {
				uint16_t value = (uint16_t)(rand() % 120);
				uint32_t delay_sub = (uint32_t)value + 1;
				if (delay_sub < delay) {
					delay -= delay_sub;
				} else {
					delay = 0;
				}
				uint16_t level_sub = (uint16_t)value / 60 + 1;
				if (level_sub < level) {
					level -= level_sub;
				} else {
					level = 0;
				}
			}
			captured_at += 56 + rand() % 8;
		}
	}

	info("seeded file %s\n", buffer_file);
	return 0;
}

int seed_config(octet_t *db) {
	uint8_t uplink_ind = 0;

	time_t captured_at = time(NULL);
	for (uint8_t index = 0; index < device_ids_len; index++) {
		config_t config = {
				.led_debug = uplink_ind % 4 == 0,
				.reading_enable = uplink_ind % 8 != 0,
				.metric_enable = uplink_ind % 4 == 0,
				.buffer_enable = uplink_ind % 4 != 0,
				.reading_interval = (uint16_t)(30 + rand() % 30),
				.metric_interval = (uint16_t)(60 + rand() % 60),
				.buffer_interval = (uint16_t)(15 + rand() % 15),
				.captured_at = captured_at,
				.device_id = &device_ids[index],
		};

		if (config_insert(db, &config) != 0) {
			return -1;
		}

		captured_at -= 50 + rand() % 20;
		uplink_ind += 1;
	}

	info("seeded file %s\n", config_file);
	return 0;
}

int seed_radio(octet_t *db) {
	time_t captured_at = time(NULL);
	for (uint8_t index = 0; index < device_ids_len; index++) {
		radio_t radio = {
				.frequency = (uint32_t)(433225000 + (rand() % 7) * 200000),
				.bandwidth = (uint32_t)(62500 + (rand() % 4) * 62500),
				.coding_rate = (uint8_t)(5 + rand() % 4),
				.spreading_factor = (uint8_t)(6 + rand() % 7),
				.preamble_length = (uint8_t)(6 + rand() % 16),
				.tx_power = (uint8_t)(2 + rand() % 16),
				.sync_word = 0x12,
				.checksum = rand() % 2,
				.captured_at = captured_at,
				.device_id = &device_ids[index],
		};

		if (radio_insert(db, &radio) != 0) {
			return -1;
		}

		captured_at -= 50 + rand() % 20;
	}

	info("seeded file %s\n", radio_file);
	return 0;
}

int seed_uplink(octet_t *db) {
	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint16_t frame = 0;
		int16_t rssi = (int16_t)(rand() % 128 - 157);
		int8_t snr = (int8_t)(rand() % 144 - 96);
		uint8_t sf = (uint8_t)(rand() % 7 + 6);
		uint8_t cr = (uint8_t)(rand() % 4 + 5);
		uint8_t tx_power = (uint8_t)(rand() % 16 + 2);
		uint8_t preamble_len = (uint8_t)(rand() % 16 + 6);
		time_t now = time(NULL);
		time_t received_at = time(NULL) - 2 * 24 * 60 * 60;
		while (received_at < now) {
			uint8_t data[16];
			uint8_t data_len = 4 + (uint8_t)(rand() % 12);
			for (uint8_t ind = 0; ind < data_len; ind++) {
				data[ind] = (uint8_t)rand();
			}
			uplink_t uplink = {
					.frame = frame,
					.kind = (uint8_t)rand(),
					.data = data,
					.data_len = data_len,
					.airtime = 12 * 16 + (uint16_t)(rand() % 256),
					.frequency = (uint32_t)(435625 * 1000 - sf * 200 * 1000),
					.bandwidth = 125 * 1000,
					.rssi = rssi,
					.snr = snr,
					.sf = sf,
					.cr = cr,
					.tx_power = tx_power,
					.preamble_len = preamble_len,
					.received_at = received_at,
					.device_id = &device_ids[index],
			};

			if (uplink_insert(db, &uplink) != 0) {
				return -1;
			}
			frame += 1;
			rssi += (int16_t)(rand() % 5 - 2);
			if (rssi < -157) {
				rssi += 10;
			}
			if (rssi > -29) {
				rssi -= 10;
			}
			snr += (int8_t)(rand() % 5 - 2);
			if (snr < -96) {
				snr += 10;
			}
			if (snr > 48) {
				snr -= 10;
			}
			sf += (uint8_t)(rand() % 3 - 1);
			if (sf < 6) {
				sf += 1;
			}
			if (sf > 12) {
				sf -= 1;
			}
			cr += (uint8_t)(rand() % 3 - 1);
			if (cr < 5) {
				cr += 1;
			}
			if (cr > 8) {
				cr -= 1;
			}
			tx_power += (uint8_t)(rand() % 3 - 1);
			if (tx_power < 2) {
				tx_power += 1;
			}
			if (tx_power > 17) {
				tx_power -= 1;
			}
			preamble_len += (uint8_t)(rand() % 3 - 1);
			if (preamble_len < 6) {
				preamble_len += 1;
			}
			if (preamble_len > 21) {
				preamble_len -= 1;
			}
			received_at += 56 + rand() % 8;
		}
	}

	info("seeded file %s\n", uplink_file);
	return 0;
}

int seed_downlink(octet_t *db) {
	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint16_t frame = 0;
		uint8_t sf = (uint8_t)(rand() % 7 + 6);
		uint8_t cr = (uint8_t)(rand() % 4 + 5);
		uint8_t tx_power = (uint8_t)(rand() % 16 + 2);
		uint8_t preamble_len = (uint8_t)(rand() % 16 + 6);
		time_t now = time(NULL);
		time_t sent_at = time(NULL) - 2 * 24 * 60 * 60;
		while (sent_at < now) {
			uint8_t data[16];
			uint8_t data_len = 4 + (uint8_t)(rand() % 12);
			for (uint8_t ind = 0; ind < data_len; ind++) {
				data[ind] = (uint8_t)rand();
			}
			downlink_t downlink = {
					.frame = frame,
					.kind = (uint8_t)rand(),
					.data = data,
					.data_len = data_len,
					.airtime = 12 * 16 + (uint16_t)(rand() % 256),
					.frequency = (uint32_t)(435625 * 1000 - sf * 200 * 1000),
					.bandwidth = 125 * 1000,
					.sf = sf,
					.cr = cr,
					.tx_power = tx_power,
					.preamble_len = preamble_len,
					.sent_at = sent_at,
					.device_id = &device_ids[index],
			};

			if (downlink_insert(db, &downlink) != 0) {
				return -1;
			}
			frame += 1;
			sf += (uint8_t)(rand() % 3 - 1);
			if (sf < 6) {
				sf += 1;
			}
			if (sf > 12) {
				sf -= 1;
			}
			cr += (uint8_t)(rand() % 3 - 1);
			if (cr < 5) {
				cr += 1;
			}
			if (cr > 8) {
				cr -= 1;
			}
			tx_power += (uint8_t)(rand() % 3 - 1);
			if (tx_power < 2) {
				tx_power += 1;
			}
			if (tx_power > 17) {
				tx_power -= 1;
			}
			preamble_len += (uint8_t)(rand() % 3 - 1);
			if (preamble_len < 6) {
				preamble_len += 1;
			}
			if (preamble_len > 21) {
				preamble_len -= 1;
			}
			sent_at += 56 + rand() % 8;
		}
	}

	info("seeded file %s\n", downlink_file);
	return 0;
}

int seed(octet_t *db) {
	if (seed_user(db) == -1) {
		return -1;
	}
	if (seed_zone(db) == -1) {
		return -1;
	}
	if (seed_user_zone(db) == -1) {
		return -1;
	}
	if (seed_device(db) == -1) {
		return -1;
	}
	if (seed_user_device(db) == -1) {
		return -1;
	}
	if (seed_host(db) == -1) {
		return -1;
	}
	if (seed_reading(db) == -1) {
		return -1;
	}
	if (seed_metric(db) == -1) {
		return -1;
	}
	if (seed_buffer(db) == -1) {
		return -1;
	}
	if (seed_config(db) == -1) {
		return -1;
	}
	if (seed_radio(db) == -1) {
		return -1;
	}
	if (seed_uplink(db) == -1) {
		return -1;
	}
	if (seed_downlink(db) == -1) {
		return -1;
	}

	free(user_ids);
	free(zone_ids);
	free(device_ids);

	return 0;
}
