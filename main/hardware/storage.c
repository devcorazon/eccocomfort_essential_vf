/*
 * storage.c
 *
 *  Created on: 1 dic 2022
 *      Author: Daniele Schirosi
 */

#include <freertos/FreeRTOS.h>

#include "nvs_flash.h"
#include "esp_efuse.h"

#include "string.h"

#include "system.h"
#include "storage_internal.h"

//#TODO other defines
#define MODE_SET_KEY		"mode_set"

static nvs_handle_t storage_handle;

static int storage_serial_number_obtain(void);
static int storage_read_entry_with_idx(size_t i);
static int storage_save_entry_with_key(const char* key);
static int storage_save_all_entry(void);

// Data on ram.
//static __attribute__((section(".noinit"))) struct application_data_s application_data;
static struct application_data_s application_data;

static struct storage_entry_s storage_entry_poll[] = {
		{ MODE_SET_KEY,		&application_data.configuration_settings.mode_set,						DATA_TYPE_UINT8, 	1 },
		{ "speed_set",		&application_data.configuration_settings.speed_set,						DATA_TYPE_UINT8, 	1 },

		{ "r_hum_set",		&application_data.configuration_settings.relative_humidity_set,			DATA_TYPE_UINT8, 	1 },
		{ "lux_set",		&application_data.configuration_settings.lux_set,						DATA_TYPE_UINT8, 	1 },
		{ "voc_set",		&application_data.configuration_settings.voc_set,						DATA_TYPE_UINT8, 	1 },

		{ "temp_offset",	&application_data.configuration_settings.temperature_offset,			DATA_TYPE_INT16, 	2 },
		{ "r_hum_offset",	&application_data.configuration_settings.relative_humidity_offset,		DATA_TYPE_INT16, 	2 },
};


static void storage_init_noinit_data(void) {
    if (application_data.crc_noinit_data != crc((const uint8_t *) &application_data.noinit_data, sizeof(application_data.noinit_data))) {
    	memset(&application_data.runtime_data, 0, sizeof(application_data.runtime_data));

    	application_data.crc_noinit_data = crc((const uint8_t *) &application_data.noinit_data, sizeof(application_data.noinit_data));
    }
}

static void storage_init_runtime_data(void) {
	memset(&application_data.runtime_data, 0, sizeof(application_data.runtime_data));

//#TODO
	application_data.runtime_data.temperature = TEMPERATURE_INVALID;
	application_data.runtime_data.relative_humidity = RELATIVE_HUMIDITY_INVALID;
	application_data.runtime_data.voc = VOC_INVALID;
}

static void storage_init_configuration_settings(void) {
	memset(&application_data.configuration_settings, 0, sizeof(application_data.configuration_settings));

//#TODO
	// Example
	application_data.configuration_settings.relative_humidity_set = RH_THRESHOLD_SETTING_MEDIUM;
	application_data.configuration_settings.voc_set = VOC_THRESHOLD_SETTING_MEDIUM;
	application_data.configuration_settings.lux_set = LUMINOSITY_SENSOR_SETTING_MEDIUM;
}

int storage_init(void) {
	esp_err_t ret;

	storage_init_noinit_data();
    storage_init_runtime_data();
    storage_init_configuration_settings();

    storage_serial_number_obtain();

    ret = nvs_flash_init();
    if ((ret == ESP_ERR_NVS_NO_FREE_PAGES) || (ret == ESP_ERR_NVS_NEW_VERSION_FOUND)) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ret = nvs_open("storage", NVS_READWRITE, &storage_handle);
    if (ret == ESP_OK) {
    	for (size_t i = 0u; i < ARRAY_SIZE(storage_entry_poll); i++ ) {
    		storage_read_entry_with_idx(i);
    	}
    }
    else {
    	printf("nvs_open - ERROR\r\n");
    }

    return 0;
}

int storage_set_default(void) {
	memset(&application_data, 0, sizeof(application_data));

	storage_init_noinit_data();
	storage_init_runtime_data();
	storage_init_configuration_settings();
	storage_save_all_entry();

	return 0;
}

/// runtime data

uint32_t get_serial_number(void) {
	return application_data.runtime_data.serial_number;
}

int set_serial_number(uint32_t serial_number) {
	application_data.runtime_data.serial_number = serial_number;

	return 0;
}

uint16_t get_fw_version(void) {
	return application_data.runtime_data.fw_version_v_ctrl = FIRMWARE_VERSION;
}

uint8_t get_mode_state(void) {
	return application_data.runtime_data.mode_state;
}

int set_mode_state(uint8_t mode_state) {
	if ((mode_state & 0x3f) > MODE_AUTOMATIC_CYCLE) {

		return -1;
	}

	application_data.runtime_data.mode_state = mode_state;

	return 0;
}

uint8_t get_speed_state(void) {
	return application_data.runtime_data.speed_state;
}

int set_speed_state(uint8_t speed_state) {
	if ((speed_state & 0x3f) > SPEED_BOOST) {

		return -1;
	}

	application_data.runtime_data.speed_state = speed_state;

	return 0;
}

uint8_t get_direction_state(void) {
	return application_data.runtime_data.direction_state;
}

int set_direction_state(uint8_t direction_state) {
	application_data.runtime_data.direction_state= direction_state;

	return 0;
}

uint8_t get_device_state(void) {
	return application_data.runtime_data.device_state;
}

int set_device_state(uint8_t device_state) {
	application_data.runtime_data.device_state= device_state;

	return 0;
}

int16_t get_temperature(void) {
	return application_data.runtime_data.temperature;
}

int set_temperature(int16_t temperature) {
	application_data.runtime_data.temperature = temperature;

	return 0;
}

uint16_t get_relative_humidity(void) {
	return application_data.runtime_data.relative_humidity;
}

int set_relative_humidity(uint16_t relative_humidity) {
	application_data.runtime_data.relative_humidity = relative_humidity;

	return 0;
}

uint16_t get_voc(void) {
	return application_data.runtime_data.voc;
}

int set_voc(uint16_t voc) {
	application_data.runtime_data.voc = voc;

	return 0;
}

int16_t get_lux(void) {
	return application_data.runtime_data.lux;
}

int set_lux(int16_t lux) {
	application_data.runtime_data.lux = lux;

	return 0;
}

int16_t get_internal_temperature(void) {
	return application_data.runtime_data.internal_temperature;
}

int set_internal_temperature(int16_t temperature) {
	application_data.runtime_data.internal_temperature = temperature;

	return 0;
}

int16_t get_external_temperature(void) {
	return application_data.runtime_data.external_temperature;
}

int set_external_temperature(int16_t temperature) {
	application_data.runtime_data.external_temperature = temperature;

	return 0;
}

/// configuration settings
uint8_t get_mode_set(void) {
	return application_data.configuration_settings.mode_set;
}

int set_mode_set(uint8_t mode_set) {
	application_data.configuration_settings.mode_set = mode_set;

	storage_save_entry_with_key(MODE_SET_KEY);

	return 0;
}

uint8_t get_speed_set(void) {
	return application_data.configuration_settings.speed_set;
}

int set_speed_set(uint8_t speed_set) {
	application_data.configuration_settings.speed_set = speed_set;

	storage_save_entry_with_key("speed_set");

	return 0;
}


uint8_t get_relative_humidity_set(void) {
	return application_data.configuration_settings.relative_humidity_set;
}

int set_relative_humidity_set(uint8_t relative_humidity_set) {
	if (relative_humidity_set > RH_THRESHOLD_SETTING_HIGH) {
		return -1;
	}

	application_data.configuration_settings.relative_humidity_set = relative_humidity_set;
	storage_save_entry_with_key("r_hum_set");

    return 0;
}

uint8_t get_lux_set(void) {
	return application_data.configuration_settings.lux_set;
}

int set_lux_set(uint8_t lux_set) {
	if (lux_set > LUMINOSITY_SENSOR_SETTING_HIGH) {
		return -1;
	}

	application_data.configuration_settings.lux_set = lux_set;
	storage_save_entry_with_key("lux_set");

    return 0;
}

uint8_t get_voc_set(void) {
	return application_data.configuration_settings.voc_set;
}

int set_voc_set(uint8_t voc_set) {
	if (voc_set > VOC_THRESHOLD_SETTING_HIGH) {
		return -1;
	}

	application_data.configuration_settings.voc_set = voc_set;
	storage_save_entry_with_key("voc_set");

    return 0;
}

int16_t get_temperature_offset(void) {
	return application_data.configuration_settings.temperature_offset;
}

int set_temperature_offset(int16_t temperature_offset) {
	application_data.configuration_settings.temperature_offset = temperature_offset;

	storage_save_entry_with_key("temp_offset");

    return 0;
}

int16_t get_relative_humidity_offset(void) {
	return application_data.configuration_settings.relative_humidity_offset;
}

int set_relative_humidity_offset(int16_t relative_humidity_offset) {
	application_data.configuration_settings.relative_humidity_offset = relative_humidity_offset;

	storage_save_entry_with_key("r_hum_offset");

    return 0;
}

static int storage_serial_number_obtain(void) {
    uint8_t serial_number_byte[4];
    size_t start_bit = 28 * 8;
    size_t num_bits = 4 * 8;

    if (esp_efuse_read_block(EFUSE_BLK3, &serial_number_byte, start_bit, num_bits) != ESP_OK) {
    	return -1;
    }

    application_data.runtime_data.serial_number = ((uint32_t)serial_number_byte[0]) << 24 | ((uint32_t)serial_number_byte[1]) << 16 | ((uint32_t)serial_number_byte[2]) << 8 | ((uint32_t)serial_number_byte[3]);

    return 0;
}

static int storage_read_entry_with_idx(size_t i) {
//	esp_err_t ret;

//	printf("storage_read_entry_with_idx: %u - %s - %02x - %u\r\n", i, storage_entry_poll[i].key, storage_entry_poll[i].type, storage_entry_poll[i].size);

    switch(storage_entry_poll[i].type) {
        case DATA_TYPE_UINT8:
        	nvs_get_u8(storage_handle, storage_entry_poll[i].key, (uint8_t *)storage_entry_poll[i].data);
//        	ret = nvs_get_u8(storage_handle, storage_entry_poll[i].key, (uint8_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_u8 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT8:
        	nvs_get_i8(storage_handle, storage_entry_poll[i].key, (int8_t *)storage_entry_poll[i].data);
//        	ret = nvs_get_i8(storage_handle, storage_entry_poll[i].key, (int8_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_i8 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_UINT16:
        	nvs_get_u16(storage_handle, storage_entry_poll[i].key, (uint16_t *)storage_entry_poll[i].data);
//        	ret = nvs_get_u16(storage_handle, storage_entry_poll[i].key, (uint16_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_u16 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT16:
            nvs_get_i16(storage_handle, storage_entry_poll[i].key, (int16_t *)(storage_entry_poll[i].data));
//            ret = nvs_get_i16(storage_handle, storage_entry_poll[i].key, (int16_t *)(storage_entry_poll[i].data));
//        	printf("storage_save_entry_with_key - nvs_get_i16 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_UINT32:
            nvs_get_i16(storage_handle, storage_entry_poll[i].key, (int16_t *)(storage_entry_poll[i].data));
//        	ret = nvs_get_u32(storage_handle, storage_entry_poll[i].key, (uint32_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_u32 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT32:
        	nvs_get_i32(storage_handle, storage_entry_poll[i].key, (int32_t *)storage_entry_poll[i].data);
//        	ret = nvs_get_i32(storage_handle, storage_entry_poll[i].key, (int32_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_i32 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_UINT64:
        	nvs_get_u64(storage_handle, storage_entry_poll[i].key, (uint64_t *)storage_entry_poll[i].data);
//        	ret = nvs_get_u64(storage_handle, storage_entry_poll[i].key, (uint64_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_u64 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT64:
        	nvs_get_i64(storage_handle, storage_entry_poll[i].key, (int64_t *)storage_entry_poll[i].data);
//        	ret = nvs_get_i64(storage_handle, storage_entry_poll[i].key, (int64_t *)storage_entry_poll[i].data);
//        	printf("storage_save_entry_with_key - nvs_get_i64 - index: %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_STRING:
        	nvs_get_str(storage_handle, storage_entry_poll[i].key, (char *)storage_entry_poll[i].data, NULL);
//        	ret = nvs_get_str(storage_handle, storage_entry_poll[i].key, (char *)storage_entry_poll[i].data, NULL);
//        	printf("storage_save_entry_with_key - nvs_get_str - index: %u - ret: %04x\r\n", i, ret);
            break;
    }

    return 0;
}

static int storage_save_entry_with_key(const char* key) {
	size_t i;
//	esp_err_t ret;

	for (i = 0; i < ARRAY_SIZE(storage_entry_poll); i++ ) {
		if (!strcmp(key, storage_entry_poll[i].key)) {
			break;
		}
	}

	if (i == ARRAY_SIZE(storage_entry_poll)) {
    	printf("storage_save_entry_with_key - no entry\r\n");
		return -1;
	}

//	printf("storage_save_entry_with_key: %u - %s - %02x - %u\r\n", i, storage_entry_poll[i].key, storage_entry_poll[i].type, storage_entry_poll[i].size);

	switch(storage_entry_poll[i].type) {
        case DATA_TYPE_UINT8:
        	nvs_set_u8(storage_handle, storage_entry_poll[i].key, (uint8_t)*((uint8_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_u8(storage_handle, storage_entry_poll[i].key, (uint8_t)*((uint8_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_u8 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT8:
        	nvs_set_i8(storage_handle, storage_entry_poll[i].key, (int8_t)*((int8_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_i8(storage_handle, storage_entry_poll[i].key, (int8_t)*((int8_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_i8 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_UINT16:
        	nvs_set_u16(storage_handle, storage_entry_poll[i].key, (uint16_t)*((uint16_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_u16(storage_handle, storage_entry_poll[i].key, (uint16_t)*((uint16_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_u16 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT16:
        	nvs_set_i16(storage_handle, storage_entry_poll[i].key, *(int16_t *)(storage_entry_poll[i].data));
//        	ret = nvs_set_i16(storage_handle, storage_entry_poll[i].key, *(int16_t *)(storage_entry_poll[i].data));
//        	printf("storage_save_entry_with_key - nvs_set_i16 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_UINT32:
        	nvs_set_u32(storage_handle, storage_entry_poll[i].key, (uint32_t)*((uint32_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_u32(storage_handle, storage_entry_poll[i].key, (uint32_t)*((uint32_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_u32 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT32:
        	nvs_set_i32(storage_handle, storage_entry_poll[i].key, (int32_t)*((int32_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_i32(storage_handle, storage_entry_poll[i].key, (int32_t)*((int32_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_i32 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_UINT64:
        	nvs_set_u64(storage_handle, storage_entry_poll[i].key, (uint64_t)*((uint64_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_u64(storage_handle, storage_entry_poll[i].key, (uint64_t)*((uint64_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_u64 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_INT64:
        	nvs_set_i64(storage_handle, storage_entry_poll[i].key, (int64_t)*((int64_t *)(storage_entry_poll[i].data)));
//        	ret = nvs_set_i64(storage_handle, storage_entry_poll[i].key, (int64_t)*((int64_t *)(storage_entry_poll[i].data)));
//        	printf("storage_save_entry_with_key - nvs_set_i64 - index:  %u - ret: %04x\r\n", i, ret);
            break;
        case DATA_TYPE_STRING:
        	nvs_set_str(storage_handle, storage_entry_poll[i].key, (const char *)(storage_entry_poll[i].data));
//        	ret = nvs_set_str(storage_handle, storage_entry_poll[i].key, (const char *)(storage_entry_poll[i].data));
//        	printf("storage_save_entry_with_key - nvs_set_str - index:  %u - ret: %04x\r\n", i, ret);
            break;
    }

	nvs_commit(storage_handle);

    return 0;
}

static int storage_save_all_entry(void) {
	size_t i;

	for (i = 0; i < ARRAY_SIZE(storage_entry_poll); i++ ) {
		storage_save_entry_with_key(storage_entry_poll[i].key);
	}

    return 0;
}
