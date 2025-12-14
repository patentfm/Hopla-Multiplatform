/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Main application entry point
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "ble_service.h"
#include "accel_sensor.h"
#include "config_manager.h"
#include "power_mgmt.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Sensor data processing thread */
#define SENSOR_STACK_SIZE 1024
#define SENSOR_PRIORITY 5

static K_THREAD_STACK_DEFINE(sensor_stack, SENSOR_STACK_SIZE);
static struct k_thread sensor_thread_data;

/* Motion detection trigger handler */
static void motion_trigger_handler(const struct device *dev,
				   const struct sensor_trigger *trig)
{
	LOG_INF("Motion detected!");
	power_mgmt_set_state(POWER_STATE_ACTIVE);
}

/* Sensor processing thread */
static void sensor_thread(void *arg1, void *arg2, void *arg3)
{
	struct accel_data data;
	uint32_t period_ms;
	int err;

	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	while (1) {
		/* Get current configuration */
		struct fm_config config;
		config_manager_get(&config);

		/* Calculate period based on notify rate */
		period_ms = 1000 / config.notify_rate_hz;
		if (period_ms < 10) {
			period_ms = 10; /* Minimum 10ms */
		}

		/* Read sensor */
		err = accel_sensor_read(&data);
		if (err) {
			LOG_ERR("Failed to read sensor (err %d)", err);
			k_sleep(K_MSEC(period_ms));
			continue;
		}

		/* Process based on stream mode */
		uint8_t stream_mode = ble_service_get_stream_mode();

		if (stream_mode == STREAM_MODE_RAW || stream_mode == STREAM_MODE_FILTERED) {
			/* TODO: Apply filtering for FILTERED mode */
			ble_service_notify_xyz(&data);
		} else if (stream_mode == STREAM_MODE_EVENTS) {
			/* TODO: Detect events (tilt, jump, etc.) */
			/* For now, send raw data */
			ble_service_notify_xyz(&data);
		}

		/* Update power state based on activity */
		enum power_state state = power_mgmt_get_state();
		if (state == POWER_STATE_CONNECTED_IDLE) {
			/* Check if motion detected */
			int16_t magnitude = (data.x * data.x + data.y * data.y + data.z * data.z) / 1000;
			if (magnitude > 100) { /* Threshold */
				power_mgmt_set_state(POWER_STATE_CONNECTED_ACTIVE);
				power_mgmt_schedule_active_timeout(config.active_timeout_ms);
			}
		}

		k_sleep(K_MSEC(period_ms));
	}
}

/* Main function */
int main(void)
{
	int err;

	LOG_INF("Hopla Firmware Starting...");

	/* Initialize components */
	err = accel_sensor_init();
	if (err) {
		LOG_ERR("Failed to initialize accelerometer (err %d)", err);
		return err;
	}

	err = config_manager_init();
	if (err) {
		LOG_ERR("Failed to initialize config manager (err %d)", err);
		return err;
	}

	err = power_mgmt_init();
	if (err) {
		LOG_ERR("Failed to initialize power management (err %d)", err);
		return err;
	}

	err = ble_service_init();
	if (err) {
		LOG_ERR("Failed to initialize BLE service (err %d)", err);
		return err;
	}

	/* Set up motion trigger */
	const struct sensor_trigger *trigger;
	err = accel_sensor_get_trigger(&trigger);
	if (!err) {
		err = accel_sensor_set_trigger_handler(motion_trigger_handler);
		if (err) {
			LOG_WRN("Failed to set motion trigger handler (err %d)", err);
		}
	}

	/* Start advertising */
	struct fm_config config;
	config_manager_get(&config);
	err = ble_service_start_advertising(config.adv_interval_idle);
	if (err) {
		LOG_ERR("Failed to start advertising (err %d)", err);
		return err;
	}

	/* Start sensor processing thread */
	k_thread_create(&sensor_thread_data, sensor_stack,
			K_THREAD_STACK_SIZEOF(sensor_stack),
			sensor_thread, NULL, NULL, NULL,
			SENSOR_PRIORITY, 0, K_NO_WAIT);
	k_thread_name_set(&sensor_thread_data, "sensor");

	LOG_INF("Hopla Firmware Ready");

	return 0;
}

