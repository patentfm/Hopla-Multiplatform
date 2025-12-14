/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Power management profiles
 */

#include "power_mgmt.h"
#include "ble_service.h"
#include "accel_sensor.h"
#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>

LOG_MODULE_REGISTER(power_mgmt, LOG_LEVEL_DBG);

static enum power_state current_state = POWER_STATE_IDLE;
static uint16_t adv_interval = 1000;
static struct k_work_delayable active_timeout_work;

/* Active timeout handler */
static void active_timeout_handler(struct k_work *work)
{
	LOG_INF("Active timeout - switching to idle");
	power_mgmt_set_state(POWER_STATE_IDLE);
}

/* Initialize power management */
int power_mgmt_init(void)
{
	k_work_init_delayable(&active_timeout_work, active_timeout_handler);

	LOG_INF("Power management initialized");
	return 0;
}

/* Set power state */
int power_mgmt_set_state(enum power_state state)
{
	current_state = state;

	switch (state) {
	case POWER_STATE_IDLE:
		LOG_INF("State: IDLE");
		ble_service_stop_advertising();
		ble_service_start_advertising(adv_interval);
		accel_sensor_enable_wake_on_motion(true);
		break;

	case POWER_STATE_ACTIVE:
		LOG_INF("State: ACTIVE");
		ble_service_stop_advertising();
		ble_service_start_advertising(100); /* Fast advertising */
		accel_sensor_enable_wake_on_motion(false);
		break;

	case POWER_STATE_CONNECTED_IDLE:
		LOG_INF("State: CONNECTED_IDLE");
		ble_service_stop_advertising();
		accel_sensor_enable_wake_on_motion(true);
		break;

	case POWER_STATE_CONNECTED_ACTIVE:
		LOG_INF("State: CONNECTED_ACTIVE");
		ble_service_stop_advertising();
		accel_sensor_enable_wake_on_motion(false);
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* Get current power state */
enum power_state power_mgmt_get_state(void)
{
	return current_state;
}

/* Update advertising interval */
void power_mgmt_update_advertising(uint16_t interval_ms)
{
	adv_interval = interval_ms;

	if (current_state == POWER_STATE_IDLE) {
		ble_service_stop_advertising();
		ble_service_start_advertising(adv_interval);
	}
}

/* Schedule active timeout */
void power_mgmt_schedule_active_timeout(uint16_t timeout_ms)
{
	k_work_cancel_delayable(&active_timeout_work);
	k_work_schedule(&active_timeout_work, K_MSEC(timeout_ms));
}

