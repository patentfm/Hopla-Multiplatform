/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * BLE GATT Service FM_ACCEL implementation
 */

#include "ble_service.h"
#include "config_manager.h"
#include <zephyr/logging/log.h>
#include <zephyr/sys/byteorder.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_service, LOG_LEVEL_DBG);

/* UUID definitions */
static struct bt_uuid_128 fm_accel_service_uuid = BT_UUID_INIT_128(
	BT_UUID_FM_ACCEL_SERVICE_VAL);

static struct bt_uuid_128 fm_accel_xyz_uuid = BT_UUID_INIT_128(
	BT_UUID_FM_ACCEL_XYZ_VAL);

static struct bt_uuid_128 fm_accel_config_uuid = BT_UUID_INIT_128(
	BT_UUID_FM_ACCEL_CONFIG_VAL);

static struct bt_uuid_128 fm_accel_stream_mode_uuid = BT_UUID_INIT_128(
	BT_UUID_FM_ACCEL_STREAM_MODE_VAL);

static struct bt_uuid_128 fm_accel_device_info_uuid = BT_UUID_INIT_128(
	BT_UUID_FM_ACCEL_DEVICE_INFO_VAL);

/* Characteristic handles */
static struct bt_gatt_attr *xyz_char_attr;
static struct bt_conn *current_conn;

/* Configuration storage */
static struct fm_config current_config = {
	.notify_rate_hz = 50,
	.active_timeout_ms = 5000,
	.accel_range = 0, /* ACCEL_RANGE_2G */
	.motion_threshold = 50,
	.adv_interval_idle = 1000,
	.adv_interval_active = 100,
	.stream_mode = STREAM_MODE_FILTERED,
	.reserved = 0,
};

static uint8_t stream_mode = STREAM_MODE_FILTERED;

/* Device info */
static const char device_info[] = "Hopla v1.0\nHolyIOT-21014\nnRF52810";

/* XYZ notification callback */
static ssize_t read_xyz(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			void *buf, uint16_t len, uint16_t offset)
{
	/* Read not supported, use notifications */
	return BT_GATT_ERR(BT_ATT_ERR_READ_NOT_PERMITTED);
}

/* Config read callback */
static ssize_t read_config(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			   void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &current_config, sizeof(current_config));
}

/* Config write callback */
static ssize_t write_config(struct bt_conn *conn, const struct bt_gatt_attr *attr,
			    const void *buf, uint16_t len, uint16_t offset,
			    uint8_t flags)
{
	if (offset + len > sizeof(current_config)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy((uint8_t *)&current_config + offset, buf, len);

	if (offset + len == sizeof(current_config)) {
		LOG_INF("Config updated: rate=%d Hz, range=%d, mode=%d",
			current_config.notify_rate_hz,
			current_config.accel_range,
			current_config.stream_mode);
		/* Notify config manager to apply changes */
		config_manager_set(&current_config);
		config_manager_apply();
	}

	return len;
}

/* Stream mode read callback */
static ssize_t read_stream_mode(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 &stream_mode, sizeof(stream_mode));
}

/* Stream mode write callback */
static ssize_t write_stream_mode(struct bt_conn *conn,
				 const struct bt_gatt_attr *attr,
				 const void *buf, uint16_t len, uint16_t offset,
				 uint8_t flags)
{
	if (offset + len > sizeof(stream_mode)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	memcpy((uint8_t *)&stream_mode + offset, buf, len);

	if (offset + len == sizeof(stream_mode)) {
		LOG_INF("Stream mode updated: %d", stream_mode);
	}

	return len;
}

/* Device info read callback */
static ssize_t read_device_info(struct bt_conn *conn,
				const struct bt_gatt_attr *attr,
				void *buf, uint16_t len, uint16_t offset)
{
	return bt_gatt_attr_read(conn, attr, buf, len, offset,
				 device_info, strlen(device_info));
}

/* CCC changed callback */
static void xyz_ccc_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notify_enabled = (value == BT_GATT_CCC_NOTIFY);

	LOG_INF("XYZ notifications %s", notify_enabled ? "enabled" : "disabled");
}

/* GATT service definition */
BT_GATT_SERVICE_DEFINE(fm_accel_service,
	BT_GATT_PRIMARY_SERVICE(&fm_accel_service_uuid),

	/* XYZ Data Characteristic (notify) */
	BT_GATT_CHARACTERISTIC(&fm_accel_xyz_uuid.uuid,
			       BT_GATT_CHRC_NOTIFY,
			       BT_GATT_PERM_NONE,
			       read_xyz, NULL, NULL),
	BT_GATT_CCC(xyz_ccc_cfg_changed,
		    BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	/* Config Characteristic (read/write) */
	BT_GATT_CHARACTERISTIC(&fm_accel_config_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_config, write_config, NULL),

	/* Stream Mode Characteristic (read/write) */
	BT_GATT_CHARACTERISTIC(&fm_accel_stream_mode_uuid.uuid,
			       BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE,
			       BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
			       read_stream_mode, write_stream_mode, NULL),

	/* Device Info Characteristic (read) */
	BT_GATT_CHARACTERISTIC(&fm_accel_device_info_uuid.uuid,
			       BT_GATT_CHRC_READ,
			       BT_GATT_PERM_READ,
			       read_device_info, NULL, NULL),
);

/* Connection callback */
static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		LOG_ERR("Connection failed (err 0x%02x)", err);
		return;
	}

	LOG_INF("Connected");
	current_conn = bt_conn_ref(conn);
	power_mgmt_set_state(POWER_STATE_CONNECTED_IDLE);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason 0x%02x)", reason);

	if (current_conn) {
		bt_conn_unref(current_conn);
		current_conn = NULL;
	}

	power_mgmt_set_state(POWER_STATE_IDLE);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Initialize BLE service */
int ble_service_init(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		LOG_ERR("Bluetooth init failed (err %d)", err);
		return err;
	}

	LOG_INF("Bluetooth initialized");
	return 0;
}

/* Start advertising */
int ble_service_start_advertising(uint16_t interval_ms)
{
	int err;
	struct bt_le_adv_param adv_param = {
		.id = BT_ID_DEFAULT,
		.sid = 0,
		.secondary_max_skip = 0,
		.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
		.interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
		.interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
	};

	/* Convert ms to 0.625ms units */
	adv_param.interval_min = BT_GAP_ADV_INTERVAL_MIN(interval_ms);
	adv_param.interval_max = BT_GAP_ADV_INTERVAL_MAX(interval_ms);

	err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
	if (err) {
		LOG_ERR("Advertising failed to start (err %d)", err);
		return err;
	}

	LOG_INF("Advertising started (interval %d ms)", interval_ms);
	return 0;
}

/* Stop advertising */
int ble_service_stop_advertising(void)
{
	return bt_le_adv_stop();
}

/* Notify XYZ data */
int ble_service_notify_xyz(const struct accel_data *data)
{
	if (!current_conn) {
		return -ENOTCONN;
	}

	struct accel_data data_le = {
		.x = sys_cpu_to_le16(data->x),
		.y = sys_cpu_to_le16(data->y),
		.z = sys_cpu_to_le16(data->z),
	};

	return bt_gatt_notify(current_conn,
			      &fm_accel_service.attrs[1],
			      &data_le, sizeof(data_le));
}

/* Get configuration */
int ble_service_get_config(struct fm_config *config)
{
	if (!config) {
		return -EINVAL;
	}

	memcpy(config, &current_config, sizeof(current_config));
	return 0;
}

/* Set configuration */
int ble_service_set_config(const struct fm_config *config)
{
	if (!config) {
		return -EINVAL;
	}

	memcpy(&current_config, config, sizeof(current_config));
	return 0;
}

/* Get stream mode */
uint8_t ble_service_get_stream_mode(void)
{
	return stream_mode;
}

/* Set stream mode */
int ble_service_set_stream_mode(uint8_t mode)
{
	if (mode > STREAM_MODE_EVENTS) {
		return -EINVAL;
	}

	stream_mode = mode;
	return 0;
}

