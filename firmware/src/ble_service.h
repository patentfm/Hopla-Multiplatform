/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE GATT Service FM_ACCEL definitions
 */

#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <zephyr/types.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>

/* FM_ACCEL Service UUID */
#define BT_UUID_FM_ACCEL_SERVICE_VAL \
	BT_UUID_128_ENCODE(0xFACC0001, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

/* Characteristic UUIDs */
#define BT_UUID_FM_ACCEL_XYZ_VAL \
	BT_UUID_128_ENCODE(0xFAC10001, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

#define BT_UUID_FM_ACCEL_CONFIG_VAL \
	BT_UUID_128_ENCODE(0xFAC20001, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

#define BT_UUID_FM_ACCEL_STREAM_MODE_VAL \
	BT_UUID_128_ENCODE(0xFAC30001, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

#define BT_UUID_FM_ACCEL_DEVICE_INFO_VAL \
	BT_UUID_128_ENCODE(0xFAC40001, 0x0000, 0x1000, 0x8000, 0x00805f9b34fb)

/* UUID declarations */
extern struct bt_uuid_128 fm_accel_service_uuid;
extern struct bt_uuid_128 fm_accel_xyz_uuid;
extern struct bt_uuid_128 fm_accel_config_uuid;
extern struct bt_uuid_128 fm_accel_stream_mode_uuid;
extern struct bt_uuid_128 fm_accel_device_info_uuid;

/* Configuration structure (12 bytes) */
struct fm_config {
	uint16_t notify_rate_hz;      /* 1-100 Hz */
	uint16_t active_timeout_ms;   /* Timeout to idle state */
	uint8_t  accel_range;         /* 0=2G, 1=4G, 2=8G, 3=16G */
	uint8_t  motion_threshold;    /* 0-255 for wake-on-motion */
	uint16_t adv_interval_idle;   /* ms (1000-2000) */
	uint16_t adv_interval_active; /* ms (20-100) */
	uint8_t  stream_mode;         /* 0=RAW, 1=FILTERED, 2=EVENTS */
	uint8_t  reserved;
} __packed;

/* Stream modes */
#define STREAM_MODE_RAW      0
#define STREAM_MODE_FILTERED 1
#define STREAM_MODE_EVENTS   2

/* Accelerometer data structure */
struct accel_data {
	int16_t x;
	int16_t y;
	int16_t z;
} __packed;

/* Function prototypes */
int ble_service_init(void);
int ble_service_start_advertising(uint16_t interval_ms);
int ble_service_stop_advertising(void);
int ble_service_notify_xyz(const struct accel_data *data);
int ble_service_get_config(struct fm_config *config);
int ble_service_set_config(const struct fm_config *config);
uint8_t ble_service_get_stream_mode(void);
int ble_service_set_stream_mode(uint8_t mode);

#endif /* BLE_SERVICE_H */

