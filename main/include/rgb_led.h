/*
 * rgb_led.h
 *
 *  Created on: 30 juin 2023
 *      Author: youcef.benakmoume
 */

#ifndef MAIN_INCLUDE_RGB_LED_H_
#define MAIN_INCLUDE_RGB_LED_H_

#include "system.h"

#define	LED_TASK_STACK_SIZE			(configMINIMAL_STACK_SIZE * 4)
#define	LED_TASK_PRIORITY			(1)
#define	LED_TASK_PERIOD				(1000ul / portTICK_PERIOD_MS)

struct blink_params {
    uint8_t led_color;
    uint32_t blink_duration;
    uint32_t blink_period;
};

int rgb_led_init(struct i2c_dev_s *i2c_dev);
int rgb_led_set(uint8_t led_color,uint8_t led_mode);
int rgb_led_blink(uint8_t led_color, uint32_t blink_duration, uint32_t blink_period);
void rgb_led_blink_task(void *pvParameters);
void led_task(void *pvParameters);

#endif /* MAIN_INCLUDE_RGB_LED_H_ */
