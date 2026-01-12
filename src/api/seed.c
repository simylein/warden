#include "../lib/logger.h"
#include "buffer.h"
#include "config.h"
#include "device.h"
#include "downlink.h"
#include "metric.h"
#include "radio.h"
#include "reading.h"
#include "uplink.h"
#include "user-device.h"
#include "user.h"
#include "zone.h"
#include <sqlite3.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint8_t (*user_ids)[16];
uint8_t user_ids_len;
uint8_t (*zone_ids)[16];
uint8_t zone_ids_len;
uint8_t (*device_ids)[16];
uint8_t device_ids_len;
uint8_t (*uplink_ids)[16];
uint32_t uplink_ids_len;
uint8_t (*downlink_ids)[16];
uint32_t downlink_ids_len;

int seed_user(sqlite3 *database, const char *table) {
	char *usernames[] = {"alice", "bob", "charlie", "dave"};
	char *passwords[] = {".go4Alice", ".go4Bob", ".go4Charlie", ".go4Dave"};

	user_ids_len = sizeof(usernames) / sizeof(*usernames);
	user_ids = malloc(user_ids_len * sizeof(*user_ids));
	if (user_ids == NULL) {
		return -1;
	}

	for (uint8_t index = 0; index < user_ids_len; index++) {
		uint8_t permissions[4];
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

		if (user_insert(database, &user) != 0) {
			return -1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_zone(sqlite3 *database, const char *table) {
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

		if (zone_insert(database, &zone) != 0) {
			return -1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_device(sqlite3 *database, const char *table) {
	device_ids_len = (uint8_t)(8 + rand() % 16);
	device_ids = malloc(device_ids_len * sizeof(*device_ids));
	if (device_ids == NULL) {
		return -1;
	}

	for (uint8_t index = 0; index < device_ids_len; index++) {
		char name[16];
		uint8_t name_len = (uint8_t)(4 + rand() % 12);
		for (uint8_t ind = 0; ind < name_len; ind++) {
			name[ind] = (char)('a' + rand() % 26);
		}

		uint8_t (*zone_id)[16];
		if (rand() % 5 == 0) {
			zone_id = NULL;
		} else {
			zone_id = &zone_ids[rand() % 2];
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
				.firmware = firmware_len == 0 ? NULL : firmware,
				.firmware_len = firmware_len,
				.hardware = hardware_len == 0 ? NULL : hardware,
				.hardware_len = hardware_len,
				.created_at = (time_t[]){time(NULL)},
		};

		if (device_insert(database, &device) != 0) {
			return -1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_user_device(sqlite3 *database, const char *table) {
	for (uint8_t index = 0; index < user_ids_len; index++) {
		for (uint8_t ind = 0; ind < device_ids_len; ind++) {
			if (rand() % 2 == 0) {
				user_device_t user_device = {
						.user_id = &user_ids[index],
						.device_id = &device_ids[ind],
				};

				if (user_device_insert(database, &user_device) != 0) {
					return -1;
				}
			}
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_uplink(const char *table) {
	uplink_ids_len = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint16_t frame = (uint16_t)(384 + rand() % 128);
		int16_t rssi = (int16_t)(rand() % 128 - 157);
		int8_t snr = (int8_t)(rand() % 144 - 96);
		uint8_t sf = (uint8_t)(rand() % 7 + 6);
		uint8_t cr = (uint8_t)(rand() % 4 + 5);
		uint8_t tx_power = (uint8_t)(rand() % 16 + 2);
		uint8_t preamble_len = (uint8_t)(rand() % 16 + 6);
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t received_at = time(NULL);
		while (received_at > startup_at) {
			if (uplink_ids_len % 4096 == 0) {
				uplink_ids = realloc(uplink_ids, (uplink_ids_len + 4096) * sizeof(*uplink_ids));
				if (uplink_ids == NULL) {
					return -1;
				}
			}
			uint8_t data[16];
			uint8_t data_len = 4 + (uint8_t)(rand() % 12);
			for (uint8_t ind = 0; ind < data_len; ind++) {
				data[ind] = (uint8_t)rand();
			}
			uplink_t uplink = {
					.id = &uplink_ids[uplink_ids_len],
					.frame = frame,
					.kind = (uint8_t)rand(),
					.data = data,
					.data_len = data_len,
					.airtime = 12 * 16 + (uint16_t)(rand() % 4096),
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

			if (uplink_insert(&uplink) != 0) {
				return -1;
			}
			frame -= 1;
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
			received_at -= 280 + rand() % 40;
			uplink_ids_len += 1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_downlink(sqlite3 *database, const char *table) {
	downlink_ids_len = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint16_t frame = (uint16_t)(384 + rand() % 128);
		uint8_t sf = (uint8_t)(rand() % 7 + 6);
		uint8_t cr = (uint8_t)(rand() % 4 + 5);
		uint8_t tx_power = (uint8_t)(rand() % 16 + 2);
		uint8_t preamble_len = (uint8_t)(rand() % 16 + 6);
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t sent_at = time(NULL);
		while (sent_at > startup_at) {
			if (downlink_ids_len % 4096 == 0) {
				downlink_ids = realloc(downlink_ids, (downlink_ids_len + 4096) * sizeof(*downlink_ids));
				if (downlink_ids == NULL) {
					return -1;
				}
			}
			uint8_t data[16];
			uint8_t data_len = 4 + (uint8_t)(rand() % 12);
			for (uint8_t ind = 0; ind < data_len; ind++) {
				data[ind] = (uint8_t)rand();
			}
			downlink_t downlink = {
					.id = &downlink_ids[downlink_ids_len],
					.frame = frame,
					.kind = (uint8_t)rand(),
					.data = data,
					.data_len = data_len,
					.airtime = 12 * 16 + (uint16_t)(rand() % 4096),
					.frequency = (uint32_t)(435625 * 1000 - sf * 200 * 1000),
					.bandwidth = 125 * 1000,
					.sf = sf,
					.cr = cr,
					.tx_power = tx_power,
					.preamble_len = preamble_len,
					.sent_at = sent_at,
					.device_id = &device_ids[index],
			};

			if (downlink_insert(database, &downlink) != 0) {
				return -1;
			}
			frame -= 1;
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
			sent_at -= 280 + rand() % 40;
			downlink_ids_len += 1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_reading(const char *table) {
	uint32_t uplink_ind = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		float temperature = (float)(rand() % 6000) / 100 - 20;
		float humidity = (float)(rand() % 10000) / 100;
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t captured_at = time(NULL);
		while (captured_at > startup_at && uplink_ind < uplink_ids_len) {
			uint8_t id[16];
			reading_t reading = {
					.id = &id,
					.temperature = temperature,
					.humidity = humidity,
					.captured_at = captured_at,
					.device_id = &device_ids[index],
					.uplink_id = &uplink_ids[uplink_ind],
			};

			if (reading_insert(&reading) != 0) {
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
			captured_at -= 280 + rand() % 40;
			uplink_ind += 1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_metric(sqlite3 *database, const char *table) {
	uint32_t uplink_ind = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		float photovoltaic = (float)(rand() % 5000) / 1000;
		float battery = (float)(rand() % 1000) / 1000 + 3.2f;
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t captured_at = time(NULL);
		while (captured_at > startup_at && uplink_ind < uplink_ids_len) {
			uint8_t id[16];
			metric_t metric = {
					.id = &id,
					.photovoltaic = photovoltaic,
					.battery = battery,
					.captured_at = captured_at,
					.device_id = &device_ids[index],
					.uplink_id = &uplink_ids[uplink_ind],
			};

			if (metric_insert(database, &metric) != 0) {
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
			captured_at -= 280 + rand() % 40;
			uplink_ind += 1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_buffer(sqlite3 *database, const char *table) {
	uint32_t uplink_ind = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint32_t delay = 0;
		uint16_t level = 0;
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t captured_at = time(NULL);
		while (captured_at > startup_at && uplink_ind < uplink_ids_len) {
			uint8_t id[16];
			buffer_t buffer = {
					.id = &id,
					.delay = delay,
					.level = level,
					.captured_at = captured_at,
					.device_id = &device_ids[index],
					.uplink_id = &uplink_ids[uplink_ind],
			};

			if (buffer_insert(database, &buffer) != 0) {
				return -1;
			}
			bool increase = rand() % 4 == 0;
			bool decrease = rand() % 64 == 0;
			if (decrease) {
				uint32_t delay_sub = (uint32_t)(rand() % 3600);
				if (delay_sub < delay) {
					delay -= delay_sub;
				} else {
					delay = 0;
				}
				uint16_t level_sub = (uint16_t)(rand() % 60);
				if (level_sub < level) {
					level -= level;
				} else {
					level = 0;
				}
			}
			if (increase) {
				delay += (uint32_t)(rand() % 120);
				level += (uint16_t)(rand() % 2);
			}
			captured_at -= 280 + rand() % 40;
			uplink_ind += 1;
		}
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_config(sqlite3 *database, const char *table) {
	uint8_t uplink_ind = 0;

	time_t captured_at = time(NULL);
	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint8_t id[16];
		config_t config = {
				.id = &id,
				.led_debug = uplink_ind % 4 == 0,
				.reading_enable = uplink_ind % 8 != 0,
				.metric_enable = uplink_ind % 4 == 0,
				.buffer_enable = uplink_ind % 4 != 0,
				.reading_interval = (uint16_t)(30 + rand() % 30),
				.metric_interval = (uint16_t)(60 + rand() % 60),
				.buffer_interval = (uint16_t)(15 + rand() % 15),
				.captured_at = captured_at,
				.device_id = &device_ids[index],
				.uplink_id = &uplink_ids[uplink_ind],
		};

		if (config_insert(database, &config) != 0) {
			return -1;
		}

		captured_at -= 50 + rand() % 20;
		uplink_ind += 1;
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed_radio(sqlite3 *database, const char *table) {
	uint8_t uplink_ind = 0;

	time_t captured_at = time(NULL);
	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint8_t id[16];
		radio_t radio = {
				.id = &id,
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
				.uplink_id = &uplink_ids[uplink_ind],
		};

		if (radio_insert(database, &radio) != 0) {
			return -1;
		}

		captured_at -= 50 + rand() % 20;
		uplink_ind += 1;
	}

	info("seeded table %s\n", table);
	return 0;
}

int seed(sqlite3 *database) {
	srand((unsigned int)time(NULL));

	if (seed_user(database, user_table) == -1) {
		return -1;
	}
	if (seed_zone(database, zone_table) == -1) {
		return -1;
	}
	if (seed_device(database, device_table) == -1) {
		return -1;
	}
	if (seed_user_device(database, user_device_table) == -1) {
		return -1;
	}
	if (seed_uplink(uplink_table) == -1) {
		return -1;
	}
	if (seed_downlink(database, downlink_table) == -1) {
		return -1;
	}
	if (seed_reading(reading_table) == -1) {
		return -1;
	}
	if (seed_metric(database, metric_table) == -1) {
		return -1;
	}
	if (seed_buffer(database, buffer_table) == -1) {
		return -1;
	}
	if (seed_config(database, config_table) == -1) {
		return -1;
	}
	if (seed_radio(database, radio_table) == -1) {
		return -1;
	}

	free(user_ids);
	free(zone_ids);
	free(device_ids);
	free(uplink_ids);
	free(downlink_ids);

	return 0;
}
