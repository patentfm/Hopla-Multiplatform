/*
 * Copyright (c) 2024
 * SPDX-License-Identifier: Apache-2.0
 *
 * Runtime configuration manager
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "ble_service.h"

/* Function prototypes */
int config_manager_init(void);
int config_manager_get(struct fm_config *config);
int config_manager_set(const struct fm_config *config);
void config_manager_apply(void);

#endif /* CONFIG_MANAGER_H */

