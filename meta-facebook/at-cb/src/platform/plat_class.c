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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <logging/log.h>

#include "libutil.h"
#include "hal_i2c.h"
#include "pmbus.h"
#include "plat_fru.h"
#include "plat_class.h"
#include "common_i2c_mux.h"
#include "pex89000.h"
#include "plat_sensor_table.h"
#include "hal_gpio.h"
#include "plat_gpio.h"
#include "ioexp_pca9555.h"
#include "plat_dev.h"

LOG_MODULE_REGISTER(plat_class);

#define NUMBER_OF_ADC_CHANNEL 16
#define AST1030_ADC_BASE_ADDR 0x7e6e9000

static uint8_t board_revision = UNKNOWN_STAGE;
static uint8_t hsc_module = HSC_MODULE_UNKNOWN;
static uint8_t pwr_brick_module = POWER_BRICK_UNKNOWN;
static uint8_t pwr_monitor_module = POWER_MONITOR_UNKNOWN;
static uint8_t vr_module = VR_UNKNOWN;
static bool is_power_good = false;

struct ASIC_CARD_INFO asic_card_info[ASIC_CARD_COUNT] = {
  [0] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [1] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [2] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [3] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [4] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [5] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [6] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [7] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [8] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [9] = {
    .card_status = ASIC_CARD_UNKNOWN_STATUS,
	.pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
    .card_type = ASIC_CARD_UNKNOWN_TYPE,
    .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
    .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [10] = {
     .card_status = ASIC_CARD_UNKNOWN_STATUS,
	 .pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
     .card_type = ASIC_CARD_UNKNOWN_TYPE,
     .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
     .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
  [11] = {
     .card_status = ASIC_CARD_UNKNOWN_STATUS,
	 .pwr_cbl_status  = ASIC_CARD_UNKNOWN_STATUS,
     .card_type = ASIC_CARD_UNKNOWN_TYPE,
     .asic_1_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS,
     .asic_2_status = ASIC_CARD_DEVICE_UNKNOWN_STATUS, },
};

uint8_t reverse_ioexp_val(uint8_t val)
{
	uint8_t index = 0;
	uint8_t reverse_val = 0;
	uint8_t first_four = 0;
	uint8_t last_four = 0;

	for (index = 0; index < 8; ++index) {
		reverse_val = reverse_val << 1;
		if ((val & 1) != 0) {
			reverse_val += 1;
		}
		val = val >> 1;
	}
	first_four = (reverse_val & 0xF0) >> 4;
	last_four = (reverse_val & 0x0F) << 4;
	return (first_four | last_four);
}

void check_accl_device_presence_status_via_ioexp()
{
	int ret = -1;
	int retry = 5;
	uint8_t shift_offset = 0;
	uint8_t addr_index = 0;
	uint8_t card_index = 0;
	uint8_t ioexp_index = 0;
	uint8_t presence_val = 0;
	uint8_t pwr_cable_prsnt_val = 0;
	uint16_t reg_val = 0;
	uint16_t reg_val_pwr_cbl_prsnt = 0;
	uint8_t ioexp_addr[] = { IOEXP_U228_ADDR, IOEXP_U229_ADDR, IOEXP_U230_ADDR };
	I2C_MSG msg = { 0 };

	if (board_revision > EVT2_STAGE) {
		// Get ACCL card present status through CPLD
		msg.bus = I2C_BUS3;
		msg.target_addr = CPLD_ADDR;
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = CPLD_ACCL_1_6_PRESENT_OFFSET;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read cpld offset: 0x%x", CPLD_ACCL_1_6_PRESENT_OFFSET);
			return;
		}

		reg_val = msg.data[0];

		memset(&msg, 0, sizeof(I2C_MSG));
		msg.bus = I2C_BUS3;
		msg.target_addr = CPLD_ADDR;
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = CPLD_ACCL_7_12_PRESENT_OFFSET;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read cpld offset: 0x%x", CPLD_ACCL_7_12_PRESENT_OFFSET);
			return;
		}

		reg_val |= (msg.data[0] << 8);

		memset(&msg, 0, sizeof(I2C_MSG));
		msg.bus = I2C_BUS3;
		msg.target_addr = CPLD_ADDR;
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = CPLD_ACCL_1_6_POWER_CABLE_PRESENT_OFFSET;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read cpld offset: 0x%x",
				CPLD_ACCL_1_6_POWER_CABLE_PRESENT_OFFSET);
			return;
		}

		reg_val_pwr_cbl_prsnt = msg.data[0];

		memset(&msg, 0, sizeof(I2C_MSG));
		msg.bus = I2C_BUS3;
		msg.target_addr = CPLD_ADDR;
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = CPLD_ACCL_7_12_POWER_CABLE_PRESENT_OFFSET;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read cpld offset: 0x%x",
				CPLD_ACCL_7_12_POWER_CABLE_PRESENT_OFFSET);
			return;
		}

		reg_val_pwr_cbl_prsnt |= (msg.data[0] << 8);

		for (card_index = 0; card_index < ASIC_CARD_COUNT; ++card_index) {
			if (card_index < (ASIC_CARD_COUNT / 2)) {
				presence_val = reg_val & 0xFF;
				pwr_cable_prsnt_val = reg_val_pwr_cbl_prsnt & 0xFF;
				shift_offset = 0;
			} else {
				presence_val = (reg_val >> 8) & 0xFF;
				pwr_cable_prsnt_val = (reg_val_pwr_cbl_prsnt >> 8) & 0xFF;
				shift_offset = (ASIC_CARD_COUNT / 2);
			}

			if ((((presence_val >> (card_index - shift_offset))) & BIT(0)) ==
			    LOW_ACTIVE) {
				asic_card_info[card_index].card_status = ASIC_CARD_PRESENT;
				asic_card_info[card_index].card_type =
					ASIC_CARD_WITH_ARTEMIS_MODULE;
			} else {
				asic_card_info[card_index].card_status = ASIC_CARD_NOT_PRESENT;
			}

			if ((((pwr_cable_prsnt_val >> (card_index - shift_offset))) & BIT(0)) ==
			    LOW_ACTIVE) {
				asic_card_info[card_index].pwr_cbl_status = ASIC_CARD_PRESENT;
			} else {
				asic_card_info[card_index].pwr_cbl_status = ASIC_CARD_NOT_PRESENT;
			}
		}
	}

	for (addr_index = 0; addr_index < ARRAY_SIZE(ioexp_addr); ++addr_index) {
		card_index = 11 - (addr_index * IOEXP_CARD_PRESENCE_COUNT);

		memset(&msg, 0, sizeof(I2C_MSG));
		msg.bus = I2C_BUS13;
		msg.target_addr = ioexp_addr[addr_index];
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = PCA9555_INPUT_PORT_REG_1;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read ioexp addr: 0x%x, offset: 0x%x", msg.target_addr,
				msg.data[0]);
			continue;
		}

		// Reverse IOexp bit value in IOexp input register 1
		reg_val = (reverse_ioexp_val(msg.data[0]) << 8);

		memset(&msg, 0, sizeof(I2C_MSG));
		msg.bus = I2C_BUS13;
		msg.target_addr = ioexp_addr[addr_index];
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = PCA9555_INPUT_PORT_REG_0;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read ioexp addr: 0x%x, offset: 0x%x", msg.target_addr,
				msg.data[0]);
			continue;
		}

		reg_val |= msg.data[0];

		for (ioexp_index = 0; ioexp_index < IOEXP_CARD_PRESENCE_COUNT; ++ioexp_index) {
			presence_val = (reg_val >> ((IOEXP_CARD_PRESENCE_COUNT - 1 - ioexp_index) *
						    IOEXP_CARD_PRESENCE_PIN_COUNT)) &
				       IOEXP_CARD_PRESENCE_MAP_VAL;

			if (board_revision < DVT_STAGE) {
				if ((presence_val & IOEXP_CARD_PRESENT_VAL) == LOW_ACTIVE) {
					asic_card_info[card_index].card_status = ASIC_CARD_PRESENT;
					asic_card_info[card_index].card_type =
						ASIC_CARD_WITH_ARTEMIS_MODULE;
				} else {
					asic_card_info[card_index].card_status =
						ASIC_CARD_NOT_PRESENT;
				}
			}

			if ((presence_val & IOEXP_DEV_1_PRESENT_VAL) == LOW_ACTIVE) {
				asic_card_info[card_index].asic_1_status = ASIC_CARD_DEVICE_PRESENT;
				asic_card_info[card_index].card_type = ASIC_CARD_WITH_FREYA;
			} else {
				asic_card_info[card_index].asic_1_status =
					ASIC_CARD_DEVICE_NOT_PRESENT;
			}

			if ((presence_val & IOEXP_DEV_2_PRESENT_VAL) == LOW_ACTIVE) {
				asic_card_info[card_index].asic_2_status = ASIC_CARD_DEVICE_PRESENT;
				asic_card_info[card_index].card_type = ASIC_CARD_WITH_FREYA;
			} else {
				asic_card_info[card_index].asic_2_status =
					ASIC_CARD_DEVICE_NOT_PRESENT;
			}

			if (asic_card_info[card_index].card_type == ASIC_CARD_WITH_ARTEMIS_MODULE) {
				asic_card_info[card_index].asic_1_status = ASIC_CARD_DEVICE_PRESENT;
				asic_card_info[card_index].asic_2_status = ASIC_CARD_DEVICE_PRESENT;
			}

			card_index -= 1;
		}
	}
}

int init_platform_config()
{
	init_board_rev_gpio();

	// Need dymic loading GPIO table, using aspeed GPIO API to get GPIO value
	const struct device *gpio_dev;
	gpio_dev = device_get_binding("GPIO0_M_P");

	board_revision = gpio_pin_get(gpio_dev, (REV_ID0 % GPIO_GROUP_SIZE));
	board_revision |= gpio_pin_get(gpio_dev, (REV_ID1 % GPIO_GROUP_SIZE)) << 1;
	board_revision |= gpio_pin_get(gpio_dev, (REV_ID2 % GPIO_GROUP_SIZE)) << 2;

	if (gpio_pin_get(gpio_dev, (HSC_MODULE_PIN_NUM % GPIO_GROUP_SIZE))) {
		hsc_module = HSC_MODULE_LTC4286;
	} else {
		hsc_module = HSC_MODULE_ADM1272;
	}

	if (gpio_pin_get(gpio_dev, (POWER_BRICK_MODULE_PIN_NUM % GPIO_GROUP_SIZE))) {
		pwr_brick_module = POWER_BRICK_BMR3512202;
	} else {
		pwr_brick_module = POWER_BRICK_Q50SN120A1;
	}

	if (gpio_pin_get(gpio_dev, (POWER_MONITOR_PIN_NUM % GPIO_GROUP_SIZE))) {
		pwr_monitor_module = POWER_MONITOR_SQ52205_INA230;
	} else {
		pwr_monitor_module = POWER_MONITOR_INA233_SQ52205;
	}

	if (gpio_pin_get(gpio_dev, (VR_MODULE_PIN_NUM % GPIO_GROUP_SIZE))) {
		vr_module = VR_MP2985H;
	} else {
		vr_module = VR_XDPE15284D;
	}

	return 0;
}

uint8_t get_board_revision()
{
	return board_revision;
}

uint8_t get_hsc_module()
{
	return hsc_module;
}

uint8_t get_pwr_brick_module()
{
	return pwr_brick_module;
}

uint8_t get_pwr_monitor_module()
{
	return pwr_monitor_module;
}

uint8_t get_vr_module()
{
	return vr_module;
}

bool get_acb_power_status()
{
	int ret = -1;
	int retry = 5;
	bool current_power_status = false;
	I2C_MSG msg = { 0 };

	msg.bus = I2C_BUS3;
	msg.target_addr = CPLD_ADDR;
	msg.rx_len = 1;
	msg.tx_len = 1;
	msg.data[0] = CPLD_PWRGD_1_OFFSET;

	ret = i2c_master_read(&msg, retry);
	if (ret != 0) {
		LOG_ERR("Fail to read cpld offset: 0x%x", CPLD_PWRGD_1_OFFSET);
		return false;
	}

	if (msg.data[0] & CPLD_PWRGD_BIT) {
		memset(&msg, 0, sizeof(I2C_MSG));

		msg.bus = I2C_BUS3;
		msg.target_addr = CPLD_ADDR;
		msg.rx_len = 1;
		msg.tx_len = 1;
		msg.data[0] = CPLD_PWRGD_2_OFFSET;

		ret = i2c_master_read(&msg, retry);
		if (ret != 0) {
			LOG_ERR("Fail to read cpld offset: 0x%x", CPLD_PWRGD_2_OFFSET);
			return false;
		}

		if (msg.data[0] & CPLD_PWRGD_BIT) {
			current_power_status = true;
		}
	}

	if (is_power_good == true && current_power_status == false) {
		// DC drop
		uint8_t index = 0;
		for (index = 0; index < ASIC_CARD_COUNT; ++index) {
			clear_freya_cache_flag(index);
		}
	}

	is_power_good = current_power_status;

	return true;
}

bool get_acb_power_good_flag()
{
	return is_power_good;
}
