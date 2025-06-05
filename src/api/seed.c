#include "../lib/logger.h"
#include "device.h"
#include "user-device.h"
#include "user.h"
#include <sqlite3.h>

uint8_t user_ids[4][16];
uint8_t device_ids[8][16];

int seed_user(sqlite3 *database) {
	user_t users[] = {
			{.id = &user_ids[0], .username = "alice", .username_len = 5, .password = ".go4Alice", .password_len = 9},
			{.id = &user_ids[1], .username = "bob", .username_len = 3, .password = ".go4Bob", .password_len = 7},
			{.id = &user_ids[2], .username = "charlie", .username_len = 7, .password = ".go4Charlie", .password_len = 11},
			{.id = &user_ids[3], .username = "dave", .username_len = 4, .password = ".go4Dave", .password_len = 8},
	};

	for (uint8_t index = 0; index < sizeof(users) / sizeof(user_t); index++) {
		if (user_insert(database, &users[index]) != 0) {
			return -1;
		}
	}

	info("seeded table user\n");
	return 0;
}

int seed_device(sqlite3 *database) {
	device_t devices[] = {
			{.id = &device_ids[0], .name = "outside north", .name_len = 13},
			{.id = &device_ids[1], .name = "outside east", .name_len = 12},
			{.id = &device_ids[2], .name = "outside south", .name_len = 13},
			{.id = &device_ids[3], .name = "outside west", .name_len = 12},
			{.id = &device_ids[4], .name = "basement", .name_len = 8},
			{.id = &device_ids[5], .name = "living room", .name_len = 11},
			{.id = &device_ids[6], .name = "kitchen", .name_len = 7},
			{.id = &device_ids[7], .name = "bathroom", .name_len = 8},
	};

	for (uint8_t index = 0; index < sizeof(devices) / sizeof(device_t); index++) {
		if (device_insert(database, &devices[index]) != 0) {
			return -1;
		}
	}

	info("seeded table device\n");
	return 0;
}

int seed_user_device(sqlite3 *database) {
	user_device_t users_devices[] = {
			{.user_id = &user_ids[0], .device_id = &device_ids[0]}, {.user_id = &user_ids[0], .device_id = &device_ids[1]},
			{.user_id = &user_ids[0], .device_id = &device_ids[2]}, {.user_id = &user_ids[0], .device_id = &device_ids[3]},
			{.user_id = &user_ids[0], .device_id = &device_ids[4]}, {.user_id = &user_ids[0], .device_id = &device_ids[5]},
			{.user_id = &user_ids[0], .device_id = &device_ids[6]}, {.user_id = &user_ids[0], .device_id = &device_ids[7]},
			{.user_id = &user_ids[1], .device_id = &device_ids[0]}, {.user_id = &user_ids[1], .device_id = &device_ids[1]},
			{.user_id = &user_ids[1], .device_id = &device_ids[2]}, {.user_id = &user_ids[1], .device_id = &device_ids[3]},
			{.user_id = &user_ids[2], .device_id = &device_ids[5]}, {.user_id = &user_ids[2], .device_id = &device_ids[6]},
	};

	for (uint8_t index = 0; index < sizeof(users_devices) / sizeof(user_device_t); index++) {
		if (user_device_insert(database, &users_devices[index]) != 0) {
			return -1;
		}
	}

	info("seeded table user device\n");
	return 0;
}

int seed(sqlite3 *database) {
	if (seed_user(database) == -1) {
		return -1;
	}
	if (seed_device(database) == -1) {
		return -1;
	}
	if (seed_user_device(database) == -1) {
		return -1;
	}

	return 0;
}
