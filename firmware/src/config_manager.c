/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Runtime configuration manager
 */

#include "config_manager.h"
#include "accel_sensor.h"
#include "power_mgmt.h"
#include "ble_service.h"
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(config_manager, LOG_LEVEL_DBG);

static struct fm_config current_config;

/* Initialize config manager */
int config_manager_init(void)
{
	/* Set default configuration */
	current_config.notify_rate_hz = 50;
	current_config.active_timeout_ms = 5000;
	current_config.accel_range = 0; /* ACCEL_RANGE_2G */
	current_config.motion_threshold = 50;
	current_config.adv_interval_idle = 1000;
	current_config.adv_interval_active = 100;
	current_config.stream_mode = STREAM_MODE_FILTERED;
	current_config.reserved = 0;

	/* Apply initial configuration */
	config_manager_apply();

	LOG_INF("Config manager initialized");
	return 0;
}

/* Get current configuration */
int config_manager_get(struct fm_config *config)
{
	if (!config) {
		return -EINVAL;
	}

	memcpy(config, &current_config, sizeof(current_config));
	return 0;
}

/* Set configuration */
int config_manager_set(const struct fm_config *config)
{
	if (!config) {
		return -EINVAL;
	}

	/* Validate ranges */
	if (config->notify_rate_hz < 1 || config->notify_rate_hz > 100) {
		LOG_ERR("Invalid notify_rate_hz: %d", config->notify_rate_hz);
		return -EINVAL;
	}

	if (config->accel_range > 3) { /* ACCEL_RANGE_16G */
		LOG_ERR("Invalid accel_range: %d", config->accel_range);
		return -EINVAL;
	}

	if (config->stream_mode > STREAM_MODE_EVENTS) {
		LOG_ERR("Invalid stream_mode: %d", config->stream_mode);
		return -EINVAL;
	}

	memcpy(&current_config, config, sizeof(current_config));
	return 0;
}

/* Apply configuration to hardware */
void config_manager_apply(void)
{
	/* Apply accelerometer settings */
	accel_sensor_set_range((enum accel_range)current_config.accel_range);
	accel_sensor_set_odr(current_config.notify_rate_hz);
	accel_sensor_set_motion_threshold(current_config.motion_threshold);

	/* Apply stream mode */
	ble_service_set_stream_mode(current_config.stream_mode);

	/* Update advertising intervals */
	power_mgmt_update_advertising(current_config.adv_interval_idle);

	LOG_INF("Configuration applied");
}

