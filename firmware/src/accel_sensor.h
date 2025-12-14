/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Accelerometer sensor wrapper for LIS2DH12
 */

#ifndef ACCEL_SENSOR_H
#define ACCEL_SENSOR_H

#include <zephyr/types.h>
#include <zephyr/drivers/sensor.h>

/* Accelerometer data structure */
struct accel_data {
	int16_t x;
	int16_t y;
	int16_t z;
};

/* Accelerometer range */
enum accel_range {
	ACCEL_RANGE_2G = 0,
	ACCEL_RANGE_4G = 1,
	ACCEL_RANGE_8G = 2,
	ACCEL_RANGE_16G = 3,
};

/* Function prototypes */
int accel_sensor_init(void);
int accel_sensor_read(struct accel_data *data);
int accel_sensor_set_range(enum accel_range range);
int accel_sensor_set_odr(uint16_t rate_hz);
int accel_sensor_set_motion_threshold(uint8_t threshold);
int accel_sensor_enable_wake_on_motion(bool enable);
int accel_sensor_get_trigger(const struct sensor_trigger **trigger);
int accel_sensor_set_trigger_handler(sensor_trigger_handler_t handler);

#endif /* ACCEL_SENSOR_H */

