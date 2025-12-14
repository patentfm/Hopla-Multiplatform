/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Accelerometer sensor wrapper for LIS2DH12
 */

#include "accel_sensor.h"
#include <zephyr/logging/log.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>

LOG_MODULE_REGISTER(accel_sensor, LOG_LEVEL_DBG);

/* Device tree label */
#define LIS2DH_NODE DT_NODELABEL(lis2dh12)

/* Check if device is available */
#if !DT_NODE_EXISTS(LIS2DH_NODE)
#error "LIS2DH12 node not found in device tree"
#endif

static const struct device *accel_dev = DEVICE_DT_GET(LIS2DH_NODE);
static struct sensor_trigger motion_trigger;
static bool trigger_initialized = false;

/* ODR mapping */
static const uint16_t odr_map[] = {1, 10, 25, 50, 100, 200, 400, 1600};

/* Initialize accelerometer */
int accel_sensor_init(void)
{
	if (!device_is_ready(accel_dev)) {
		LOG_ERR("Accelerometer device not ready");
		return -ENODEV;
	}

	LOG_INF("Accelerometer initialized");
	return 0;
}

/* Read accelerometer data */
int accel_sensor_read(struct accel_data *data)
{
	struct sensor_value accel[3];
	int err;

	if (!data) {
		return -EINVAL;
	}

	err = sensor_sample_fetch(accel_dev);
	if (err) {
		LOG_ERR("Failed to fetch sample (err %d)", err);
		return err;
	}

	err = sensor_channel_get(accel_dev, SENSOR_CHAN_ACCEL_XYZ, accel);
	if (err) {
		LOG_ERR("Failed to get channel (err %d)", err);
		return err;
	}

	/* Convert to int16_t (milli-G units) */
	data->x = (int16_t)(sensor_value_to_milli(&accel[0]) / 1000);
	data->y = (int16_t)(sensor_value_to_milli(&accel[1]) / 1000);
	data->z = (int16_t)(sensor_value_to_milli(&accel[2]) / 1000);

	return 0;
}

/* Set accelerometer range */
int accel_sensor_set_range(enum accel_range range)
{
	struct sensor_value val;
	int err;

	if (range > ACCEL_RANGE_16G) {
		return -EINVAL;
	}

	val.val1 = range;
	val.val2 = 0;

	err = sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_FULL_SCALE, &val);
	if (err) {
		LOG_ERR("Failed to set range (err %d)", err);
		return err;
	}

	LOG_INF("Accelerometer range set to %d", range);
	return 0;
}

/* Set output data rate */
int accel_sensor_set_odr(uint16_t rate_hz)
{
	struct sensor_value val;
	int err;
	uint16_t odr = 0;

	/* Find closest ODR */
	for (int i = 0; i < ARRAY_SIZE(odr_map); i++) {
		if (odr_map[i] >= rate_hz) {
			odr = odr_map[i];
			break;
		}
	}

	if (odr == 0) {
		odr = odr_map[ARRAY_SIZE(odr_map) - 1];
	}

	val.val1 = odr;
	val.val2 = 0;

	err = sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_SAMPLING_FREQUENCY, &val);
	if (err) {
		LOG_ERR("Failed to set ODR (err %d)", err);
		return err;
	}

	LOG_INF("Accelerometer ODR set to %d Hz", odr);
	return 0;
}

/* Set motion threshold */
int accel_sensor_set_motion_threshold(uint8_t threshold)
{
	struct sensor_value val;
	int err;

	/* Convert threshold to sensor value (in milli-G) */
	val.val1 = threshold * 16; /* Scale factor */
	val.val2 = 0;

	err = sensor_attr_set(accel_dev, SENSOR_CHAN_ACCEL_XYZ,
			      SENSOR_ATTR_SLOPE_TH, &val);
	if (err) {
		LOG_ERR("Failed to set motion threshold (err %d)", err);
		return err;
	}

	LOG_INF("Motion threshold set to %d", threshold);
	return 0;
}

/* Enable/disable wake-on-motion */
int accel_sensor_enable_wake_on_motion(bool enable)
{
	int err;

	if (!trigger_initialized) {
		/* Initialize trigger */
		motion_trigger.type = SENSOR_TRIG_DELTA;
		motion_trigger.chan = SENSOR_CHAN_ACCEL_XYZ;
		trigger_initialized = true;
	}

	if (enable) {
		err = sensor_trigger_set(accel_dev, &motion_trigger,
					NULL); /* Handler set elsewhere */
	} else {
		err = sensor_trigger_set(accel_dev, &motion_trigger, NULL);
	}

	if (err) {
		LOG_ERR("Failed to %s wake-on-motion (err %d)",
			enable ? "enable" : "disable", err);
		return err;
	}

	LOG_INF("Wake-on-motion %s", enable ? "enabled" : "disabled");
	return 0;
}

/* Get trigger */
int accel_sensor_get_trigger(const struct sensor_trigger **trigger)
{
	if (!trigger) {
		return -EINVAL;
	}

	if (!trigger_initialized) {
		return -ENODATA;
	}

	*trigger = &motion_trigger;
	return 0;
}

/* Set trigger handler */
int accel_sensor_set_trigger_handler(sensor_trigger_handler_t handler)
{
	if (!trigger_initialized) {
		motion_trigger.type = SENSOR_TRIG_DELTA;
		motion_trigger.chan = SENSOR_CHAN_ACCEL_XYZ;
		trigger_initialized = true;
	}

	return sensor_trigger_set(accel_dev, &motion_trigger, handler);
}

