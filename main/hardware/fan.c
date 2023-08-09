/*
 * fan.c
 *
 *  Created on: 29 juin 2023
 *      Author: youcef.benakmoume
 */
#include "fan.h"
#include "storage.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_rom_gpio.h"

static int fan_speed = 0;

int fan_init()
{
    BaseType_t task_created = xTaskCreate(fan_task, "FAN task ", FAN_TASK_STACK_SIZE, NULL, FAN_TASK_PRIORITY, NULL);

    return task_created == pdPASS ? 0 : -1;
}

int fan_set(uint8_t direction, uint8_t speed)
{

    gpio_set_level(FAN_DIRECTION_PIN, (int)direction);

    if (speed > 5)
    {
        printf("Speed value exceeded. Setting to maximum.\n");
        speed = 5;
    }

    if (direction == FAN_IN)
    {
        fan_speed = fan_pwm_pulse_in[speed];
    }
    else
    {
        fan_speed = fan_pwm_pulse_out[speed];
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, fan_speed);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);

    set_direction_state(direction);
    set_speed_state(fan_speed);

    printf("Fan set: direction=%u, speed=%d.\n", direction, fan_speed);

    return 0;
}

void fan_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));  // Sleep for 100 ms
    }
}
