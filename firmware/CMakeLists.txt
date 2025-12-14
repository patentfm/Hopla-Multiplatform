# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hopla_firmware)

target_sources(app PRIVATE
  src/main.c
  src/ble_service.c
  src/accel_sensor.c
  src/config_manager.c
  src/power_mgmt.c
)

