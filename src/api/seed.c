#include "../lib/logger.h"
#include "device.h"
#include "uplink.h"
#include "user-device.h"
#include "user.h"
#include <sqlite3.h>

uint8_t user_ids[4][16];
uint8_t device_ids[8][16];
uint8_t uplink_ids[16][16];

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
			{.id = &device_ids[0], .name = "outside north", .name_len = 13, .type = "outdoor", .type_len = 7},
			{.id = &device_ids[1], .name = "outside east", .name_len = 12, .type = "outdoor", .type_len = 7},
			{.id = &device_ids[2], .name = "outside south", .name_len = 13, .type = "outdoor", .type_len = 7},
			{.id = &device_ids[3], .name = "outside west", .name_len = 12, .type = "outdoor", .type_len = 7},
			{.id = &device_ids[4], .name = "basement", .name_len = 8, .type = "indoor", .type_len = 6},
			{.id = &device_ids[5], .name = "living room", .name_len = 11, .type = "indoor", .type_len = 6},
			{.id = &device_ids[6], .name = "kitchen", .name_len = 7, .type = "indoor", .type_len = 6},
			{.id = &device_ids[7], .name = "bathroom", .name_len = 8, .type = "indoor", .type_len = 6},
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

int seed_uplink(sqlite3 *database) {
	uplink_t uplinks[] = {
			{
					.id = &uplink_ids[0],
					.kind = 0x00,
					.data = (uint8_t[]){0x5a, 0x3f, 0x91, 0xc2},
					.data_len = 4,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -77,
					.snr = (int8_t)(8.5 * 4),
					.sf = 7,
					.received_at = 946684800,
					.device_id = &device_ids[0],
			},
			{
					.id = &uplink_ids[1],
					.kind = 0x01,
					.data = (uint8_t[]){0xb7, 0xe9, 0x1f, 0x84, 0xd2, 0xa7, 0xc1, 0xe0},
					.data_len = 8,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -112,
					.snr = (int8_t)(-2.5 * 4),
					.sf = 7,
					.received_at = 946688400,
					.device_id = &device_ids[1],
			},
			{
					.id = &uplink_ids[2],
					.kind = 0x02,
					.data = (uint8_t[]){0x7c, 0x10, 0xa3, 0xd5, 0xb2},
					.data_len = 5,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -122,
					.snr = (int8_t)(-10.5 * 4),
					.sf = 7,
					.received_at = 946692000,
					.device_id = &device_ids[0],
			},
			{
					.id = &uplink_ids[3],
					.kind = 0x02,
					.data = (uint8_t[]){0x3f, 0x12},
					.data_len = 2,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -128,
					.snr = (int8_t)(-17.5 * 4),
					.sf = 8,
					.received_at = 946695600,
					.device_id = &device_ids[1],
			},
			{
					.id = &uplink_ids[4],
					.kind = 0x00,
					.data = (uint8_t[]){0xd8, 0x3e, 0x27, 0xfa, 0x9b},
					.data_len = 5,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -110,
					.snr = (int8_t)(-12.0 * 4),
					.sf = 9,
					.received_at = 946699200,
					.device_id = &device_ids[0],
			},
			{
					.id = &uplink_ids[5],
					.kind = 0x01,
					.data = (uint8_t[]){0x4a, 0xcf, 0x01, 0xb3, 0xa0, 0x5f, 0x77, 0xd2},
					.data_len = 8,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -98,
					.snr = (int8_t)(-9.0 * 4),
					.sf = 10,
					.received_at = 946702800,
					.device_id = &device_ids[1],
			},
			{
					.id = &uplink_ids[6],
					.kind = 0x01,
					.data = (uint8_t[]){0x92, 0xf0, 0xe1},
					.data_len = 3,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -130,
					.snr = (int8_t)(-15.0 * 4),
					.sf = 11,
					.received_at = 946706400,
					.device_id = &device_ids[0],
			},
			{
					.id = &uplink_ids[7],
					.kind = 0x02,
					.data = (uint8_t[]){0xe4, 0x7f, 0x98, 0xb2, 0x04, 0x1d},
					.data_len = 6,
					.airtime = 12 * 16,
					.frequency = 433 * 1000 * 1000,
					.bandwidth = 125 * 1000,
					.rssi = -136,
					.snr = (int8_t)(-18.5 * 4),
					.sf = 12,
					.received_at = 946710000,
					.device_id = &device_ids[1],
			},
	};

	for (uint8_t index = 0; index < sizeof(uplinks) / sizeof(uplink_t); index++) {
		if (uplink_insert(database, &uplinks[index]) != 0) {
			return -1;
		}
	}

	info("seeded table uplink\n");
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
	if (seed_uplink(database) == -1) {
		return -1;
	}

	return 0;
}
