/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Power management profiles
 */

#ifndef POWER_MGMT_H
#define POWER_MGMT_H

#include <zephyr/types.h>

/* Power states */
enum power_state {
	POWER_STATE_IDLE,
	POWER_STATE_ACTIVE,
	POWER_STATE_CONNECTED_IDLE,
	POWER_STATE_CONNECTED_ACTIVE,
};

/* Function prototypes */
int power_mgmt_init(void);
int power_mgmt_set_state(enum power_state state);
enum power_state power_mgmt_get_state(void);
void power_mgmt_update_advertising(uint16_t interval_ms);
void power_mgmt_schedule_active_timeout(uint16_t timeout_ms);

#endif /* POWER_MGMT_H */

