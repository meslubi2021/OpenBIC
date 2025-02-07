/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PLAT_CLASS_H
#define PLAT_CLASS_H

#include <stdbool.h>
#include <stdint.h>
#include "plat_i2c.h"
#include "plat_gpio.h"
#include "i2c-mux-pca954x.h"

#define ASIC_CARD_COUNT 12

#define POWER_MONITOR_PIN_NUM BOARD_ID3
#define HSC_MODULE_PIN_NUM BOARD_ID2
#define POWER_BRICK_MODULE_PIN_NUM BOARD_ID1
#define VR_MODULE_PIN_NUM BOARD_ID0

#define CPLD_ADDR (0xA0 >> 1)
#define CPLD_PWRGD_1_OFFSET 0x05
#define CPLD_PWRGD_2_OFFSET 0x06
#define CPLD_ACCLA_PWRGD_OFFSET 0x24
#define CPLD_ACCLB_PWRGD_OFFSET 0x25
#define CPLD_ACCL_7_12_POWER_CABLE_PRESENT_OFFSET 0x26
#define CPLD_ACCL_1_6_POWER_CABLE_PRESENT_OFFSET 0x27
#define CPLD_ACCL_1_6_PRESENT_OFFSET 0x3F
#define CPLD_ACCL_7_12_PRESENT_OFFSET 0x3E
#define CPLD_PWRGD_BIT BIT(0)

#define IOEXP_U228_ADDR (0x40 >> 1)
#define IOEXP_U229_ADDR (0x42 >> 1)
#define IOEXP_U230_ADDR (0x44 >> 1)
#define IOEXP_CARD_PRESENCE_COUNT 4
#define IOEXP_CARD_PRESENCE_PIN_COUNT 4
#define IOEXP_CARD_PRESENCE_MAP_VAL 0x0F
#define IOEXP_CARD_PRESENT_VAL BIT(3)
#define IOEXP_DEV_1_PRESENT_VAL BIT(1)
#define IOEXP_DEV_2_PRESENT_VAL BIT(2)

enum BOARD_REVISION_ID {
	POC_STAGE = 0b000,
	EVT1_STAGE = 0b001,
	EVT2_STAGE = 0b010,
	DVT_STAGE = 0b011,
	PVT_STAGE = 0b100,
	MP_STAGE = 0b101,
	UNKNOWN_STAGE = 0xFF,
};

enum HSC_MODULE {
	HSC_MODULE_ADM1272,
	HSC_MODULE_LTC4286,
	HSC_MODULE_UNKNOWN = 0xFF,
};

enum VR_MODULE {
	VR_XDPE15284D,
	VR_MP2985H,
	VR_UNKNOWN = 0xFF,
};

enum POWER_BRICK_MODULE {
	POWER_BRICK_Q50SN120A1,
	POWER_BRICK_BMR3512202,
	POWER_BRICK_UNKNOWN = 0xFF,
};

enum POWER_MONITOR_MODULE {
	POWER_MONITOR_INA233_SQ52205,
	POWER_MONITOR_SQ52205_INA230,
	POWER_MONITOR_UNKNOWN = 0xFF,
};

enum ASIC_CARD_STATUS {
	ASIC_CARD_NOT_PRESENT,
	ASIC_CARD_PRESENT,
	ASIC_CARD_UNKNOWN_STATUS = 0xFF,
};

enum ASIC_CARD_DEVICE_STATUS {
	ASIC_CARD_DEVICE_NOT_PRESENT,
	ASIC_CARD_DEVICE_PRESENT,
	ASIC_CARD_DEVICE_UNKNOWN_STATUS = 0xFF,
};

enum ASIC_CARD_TYPE {
	ASIC_CARD_WITH_FREYA,
	ASIC_CARD_WITH_ARTEMIS_MODULE,
	ASIC_CARD_UNKNOWN_TYPE = 0xFF,
};

enum FIO_STATUS {
	FIO_NOT_PRESENT,
	FIO_PRESENT,
};

enum PCIE_DEVICE_ID {
	PCIE_DEVICE_ID1,
	PCIE_DEVICE_ID2,
	PCIE_DEVICE_ID3,
};

enum PCIE_CARD_INDEX {
	PCIE_CARD_1,
	PCIE_CARD_2,
	PCIE_CARD_3,
	PCIE_CARD_4,
	PCIE_CARD_5,
	PCIE_CARD_6,
	PCIE_CARD_7,
	PCIE_CARD_8,
	PCIE_CARD_9,
	PCIE_CARD_10,
	PCIE_CARD_11,
	PCIE_CARD_12,
};

struct ASIC_CARD_INFO {
	bool card_status;
	bool pwr_cbl_status;
	uint8_t card_type;
	bool asic_1_status;
	bool asic_2_status;
};

extern struct ASIC_CARD_INFO asic_card_info[ASIC_CARD_COUNT];

void check_accl_device_presence_status_via_ioexp();
int init_platform_config();
uint8_t get_board_revision();
uint8_t get_hsc_module();
uint8_t get_vr_module();
uint8_t get_pwr_brick_module();
uint8_t get_pwr_monitor_module();
bool get_acb_power_status();
bool get_acb_power_good_flag();

#endif
