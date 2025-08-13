#include "../lib/logger.h"
#include "device.h"
#include "downlink.h"
#include "uplink.h"
#include "user-device.h"
#include "user.h"
#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint8_t *user_ids;
uint8_t user_ids_len;
uint8_t *device_ids;
uint8_t device_ids_len;
uint8_t *uplink_ids;
uint32_t uplink_ids_len;
uint8_t *downlink_ids;
uint32_t downlink_ids_len;

int seed_user(sqlite3 *database) {
	char *usernames[] = {"alice", "bob", "charlie", "dave"};
	char *passwords[] = {".go4Alice", ".go4Bob", ".go4Charlie", ".go4Dave"};

	user_ids_len = sizeof(usernames) / sizeof(*usernames);
	user_ids = malloc(user_ids_len * sizeof(*((user_t *)0)->id));
	if (user_ids == NULL) {
		return -1;
	}

	for (uint8_t index = 0; index < user_ids_len; index++) {
		uint8_t permissions[4];
		user_t user = {
				.id = (uint8_t (*)[16])(&user_ids[index * sizeof(*((user_t *)0)->id)]),
				.username = usernames[index],
				.username_len = (uint8_t)strlen(usernames[index]),
				.password = passwords[index],
				.password_len = (uint8_t)strlen(passwords[index]),
				.permissions = &permissions,
		};

		if (user_insert(database, &user) != 0) {
			return -1;
		}
	}

	info("seeded table user\n");
	return 0;
}

int seed_device(sqlite3 *database) {
	device_ids_len = (uint8_t)(8 + rand() % 16);
	device_ids = malloc(device_ids_len * sizeof(*((device_t *)0)->id));
	if (device_ids == NULL) {
		return -1;
	}

	for (uint8_t index = 0; index < device_ids_len; index++) {
		char name[16];
		uint8_t name_len = (uint8_t)(4 + rand() % 12);
		for (uint8_t ind = 0; ind < name_len; ind++) {
			name[ind] = (char)('a' + rand() % 26);
		}

		char *type;
		uint8_t type_len;
		if (rand() % 2 == 0) {
			type = "indoor";
			type_len = 6;
		} else {
			type = "outdoor";
			type_len = 7;
		}

		device_t device = {
				.id = (uint8_t (*)[16])(&device_ids[index * sizeof(*((device_t *)0)->id)]),
				.name = name,
				.name_len = name_len,
				.type = type,
				.type_len = type_len,
		};

		if (device_insert(database, &device) != 0) {
			return -1;
		}
	}

	info("seeded table device\n");
	return 0;
}

int seed_user_device(sqlite3 *database) {
	for (uint8_t index = 0; index < user_ids_len; index++) {
		for (uint8_t ind = 0; ind < device_ids_len; ind++) {
			if (rand() % 2 == 0) {
				user_device_t user_device = {
						.user_id = (uint8_t (*)[16])(&user_ids[index * sizeof(*((user_t *)0)->id)]),
						.device_id = (uint8_t (*)[16])(&device_ids[ind * sizeof(*((device_t *)0)->id)]),
				};

				if (user_device_insert(database, &user_device) != 0) {
					return -1;
				}
			}
		}
	}

	info("seeded table user device\n");
	return 0;
}

int seed_uplink(sqlite3 *database) {
	uplink_ids_len = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		int16_t rssi = (int16_t)(rand() % 128 - 157);
		int8_t snr = (int8_t)(rand() % 144 - 96);
		uint8_t sf = (uint8_t)(rand() % 5 + 7);
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t received_at = time(NULL);
		while (received_at > startup_at) {
			if (uplink_ids_len % 4096 == 0) {
				uplink_ids = realloc(uplink_ids, (uplink_ids_len + 4096) * sizeof(*((uplink_t *)0)->id));
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
					.id = (uint8_t (*)[16])(&uplink_ids[uplink_ids_len * sizeof(*((uplink_t *)0)->id)]),
					.kind = (uint8_t)rand(),
					.data = data,
					.data_len = data_len,
					.airtime = 12 * 16 + (uint16_t)(rand() % 4096),
					.frequency = (uint32_t)(435625 * 1000 - sf * 200 * 1000),
					.bandwidth = 125 * 1000,
					.rssi = rssi,
					.snr = snr,
					.sf = sf,
					.received_at = received_at,
					.device_id = (uint8_t (*)[16])(&device_ids[index * sizeof(*((device_t *)0)->id)]),
			};

			if (uplink_insert(database, &uplink) != 0) {
				return -1;
			}
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
			if (sf < 7) {
				sf += 1;
			}
			if (sf > 12) {
				sf -= 1;
			}
			received_at -= 280 + rand() % 40;
			uplink_ids_len += 1;
		}
	}

	info("seeded table uplink\n");
	return 0;
}

int seed_downlink(sqlite3 *database) {
	downlink_ids_len = 0;

	for (uint8_t index = 0; index < device_ids_len; index++) {
		uint8_t tx_power = (uint8_t)(rand() % 16 + 2);
		uint8_t sf = (uint8_t)(rand() % 5 + 7);
		time_t startup_at = time(NULL) - 24 * 60 * 60;
		time_t sent_at = time(NULL);
		while (sent_at > startup_at) {
			if (downlink_ids_len % 4096 == 0) {
				downlink_ids = realloc(downlink_ids, (downlink_ids_len + 4096) * sizeof(*((downlink_t *)0)->id));
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
					.id = (uint8_t (*)[16])(&downlink_ids[downlink_ids_len * sizeof(*((downlink_t *)0)->id)]),
					.kind = (uint8_t)rand(),
					.data = data,
					.data_len = data_len,
					.airtime = 12 * 16 + (uint16_t)(rand() % 4096),
					.frequency = (uint32_t)(435625 * 1000 - sf * 200 * 1000),
					.bandwidth = 125 * 1000,
					.tx_power = tx_power,
					.sf = sf,
					.sent_at = sent_at,
					.device_id = (uint8_t (*)[16])(&device_ids[index * sizeof(*((device_t *)0)->id)]),
			};

			if (downlink_insert(database, &downlink) != 0) {
				return -1;
			}
			tx_power += (uint8_t)(rand() % 3 - 1);
			if (tx_power < 2) {
				tx_power += 1;
			}
			if (tx_power > 17) {
				tx_power -= 1;
			}
			sf += (uint8_t)(rand() % 3 - 1);
			if (sf < 7) {
				sf += 1;
			}
			if (sf > 12) {
				sf -= 1;
			}
			sent_at -= 280 + rand() % 40;
			downlink_ids_len += 1;
		}
	}

	info("seeded table downlink\n");
	return 0;
}

int seed(sqlite3 *database) {
	srand((unsigned int)time(NULL));

	if (seed_user(database) == -1) {
		return -1;
	}
	if (seed_device(database) == -1) {
		return -1;
	}
	if (seed_user_device(database) == -1) {
		return -1;
	}
	if (seed_uplink(database) == -1) {
		return -1;
	}
	if (seed_downlink(database) == -1) {
		return -1;
	}

	free(user_ids);
	free(device_ids);
	free(uplink_ids);
	free(downlink_ids);

	return 0;
}
