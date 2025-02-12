#include "plat_class.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hal_gpio.h"
#include "hal_i2c.h"
#include "libutil.h"
#include "plat_gpio.h"
#include "plat_i2c.h"
#include "plat_sensor_table.h"

#include <logging/log.h>

LOG_MODULE_REGISTER(plat_class);

/* ADC information for each channel
 * offset: register offset
 * shift: data of channel
 */
struct ADC_INFO {
	long unsigned int offset;
	int shift;
};

struct ADC_INFO adc_info[NUMBER_OF_ADC_CHANNEL] = {
	{ 0x10, 0 },  { 0x10, 16 },  { 0x14, 0 },  { 0x14, 16 },  { 0x18, 0 },	{ 0x18, 16 },
	{ 0x1C, 0 },  { 0x1C, 16 },  { 0x110, 0 }, { 0x110, 16 }, { 0x114, 0 }, { 0x114, 16 },
	{ 0x118, 0 }, { 0x118, 16 }, { 0x11C, 0 }, { 0x11C, 16 }
};

enum ADC_REF_VOL_SELECTION {
	REF_VOL_2_5V = 0x0, // 2.5V reference voltage selection
	REF_VOL_1_2V = 0x40 // 1.2V reference voltage selection
};

/* The condition is used for 1OU card mapping table
 * "LOWER" means the voltage is lower than the setting value
 * "HIGHER" means the voltage is higher than the setting value
 * "RANGE" means the voltage is within a range
 */
enum CONDITION {
	LOWER = 0x0,
	HIGHER = 0x01,
	RANGE = 0x02,
};

struct _SLOT_EID_MAPPING_TABLE {
	float voltage;
	uint8_t condition;
	uint8_t slot_eid;
};

struct _SLOT_EID_MAPPING_TABLE _slot_eid_mapping_table[] = {
	{ 0.1, LOWER, SLOT1 }, { 0.3, RANGE, SLOT2 }, { 0.6, RANGE, SLOT3 }, { 0.9, RANGE, SLOT4 },
	{ 1.2, RANGE, SLOT5 }, { 1.5, RANGE, SLOT6 }, { 1.8, RANGE, SLOT7 }, { 2.1, RANGE, SLOT8 },
};

uint8_t slot_eid = 0;
uint8_t get_slot_eid()
{
	return slot_eid;
}

bool get_adc_voltage(int channel, float *voltage)
{
	if (!voltage) {
		return false;
	}

	if (channel >= NUMBER_OF_ADC_CHANNEL) {
		LOG_ERR("Invalid ADC channel-%d", channel);
		return false;
	}

	uint32_t raw_value, reg_value;
	long unsigned int engine_control = 0x0;
	float reference_voltage = 0.0f;

	/* Get ADC reference voltage from Aspeed chip
	 * ADC000: Engine Control
	 * [7:6] Reference Voltage Selection
	 * 00b - 2.5V / 01b - 1.2V / 10b and 11b - External Voltage
	 */
	reg_value = sys_read32(AST1030_ADC_BASE_ADDR + engine_control);
	switch (reg_value & (BIT(7) | BIT(6))) {
	case REF_VOL_2_5V:
		reference_voltage = 2.5;
		break;
	case REF_VOL_1_2V:
		reference_voltage = 1.2;
		break;
	default:
		LOG_ERR("Unsupported the external reference voltage");
		return false;
	}

	// Read ADC raw value
	reg_value = sys_read32(AST1030_ADC_BASE_ADDR + adc_info[channel].offset);
	raw_value = (reg_value >> adc_info[channel].shift) & 0x3FF; // 10-bit(0x3FF) resolution

	// Real voltage = raw data * reference voltage / 2 ^ resolution(10)
	*voltage = (raw_value * reference_voltage) / 1024;

	return true;
}

void init_platform_config()
{
	float voltage;
	bool success = get_adc_voltage(CHANNEL_13, &voltage);
	if (success) {
		for (int i = 0; i < ARRAY_SIZE(_slot_eid_mapping_table); i++) {
			float typical_voltage = _slot_eid_mapping_table[i].voltage;
			switch (_slot_eid_mapping_table[i].condition) {
			case LOWER:
				if (voltage <= typical_voltage) {
					slot_eid = _slot_eid_mapping_table[i].slot_eid;
				}
				break;
			case HIGHER:
				if (voltage >= typical_voltage) {
					slot_eid = _slot_eid_mapping_table[i].slot_eid;
				}
				break;
			case RANGE:
				if ((voltage > typical_voltage - (typical_voltage * 0.07)) &&
				    (voltage < typical_voltage + (typical_voltage * 0.07))) {
					slot_eid = _slot_eid_mapping_table[i].slot_eid;
				}
				break;
			default:
				LOG_ERR("Unknown condition 0x%x",
					_slot_eid_mapping_table[i].condition);
				break;
			}
		}
	}
}
