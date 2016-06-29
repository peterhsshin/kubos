/*
 * KubOS RT
 * Copyright (C) 2016 Kubos Corporation
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
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"

#include "kubos-hal/gpio.h"
#include "kubos-hal/uart.h"
#include "kubos-hal/i2c.h"
#include "kubos-core/modules/klog.h"

#ifdef YOTTA_CFG_SENSORS_HTU21D
#include "kubos-core/modules/sensors/htu21d.h"
#endif

void task_i2c(void *p) {
    static int x = 0;
    int ret;

/**
 * Example of directly using the Kubos-HAL i2c interface
 */
#if !defined(YOTTA_CFG_SENSORS_HTU21D) && !defined(YOTTA_CFG_SENSORS_BNO055)
    #define I2C_DEV K_I2C1
    #define I2C_SLAVE_ADDR 0x40

    uint8_t cmd = 0xE3;
    uint8_t buffer[3];

    KI2CConf conf = {
        .addressing_mode = K_ADDRESSINGMODE_7BIT,
        .role = K_MASTER,
        .clock_speed = 10000
    };
    // Initialize first i2c bus with configuration
    k_i2c_init(I2C_DEV, &conf);
    // Send single byte command to slave
    k_i2c_write(I2C_DEV, I2C_SLAVE_ADDR, &cmd, 1);
    // Processing delay
    vTaskDelay(50);
    // Read back 3 byte response from slave
    k_i2c_read(I2C_DEV, I2C_SLAVE_ADDR, &buffer, 3);

/**
 * If any sensors are detected then we will use those instead
 */
#else

#ifdef YOTTA_CFG_SENSORS_HTU21D
    float temp, hum;
    htu21d_setup();
#endif

#ifdef YOTTA_CFG_SENSORS_BNO055
    bno055_setup();
    uint32_t raw;
#endif

    while (1) {
#ifdef YOTTA_CFG_SENSORS_HTU21D
        temp = htu21d_read_temperature();
        hum = htu21d_read_humidity();
        printf("temp - %f\r\n", temp);
        printf("humidity - %f\r\n", hum);
#endif

#ifdef YOTTA_CFG_SENSORS_BNO055
        raw = bno055_read_raw();
        printf("raw imu %d\r\n", raw);
#endif

        vTaskDelay(100 / portTICK_RATE_MS);
    }
#endif
}

int main(void)
{
    k_uart_console_init();

    #ifdef TARGET_LIKE_STM32
    k_gpio_init(K_LED_GREEN, K_GPIO_OUTPUT, K_GPIO_PULL_NONE);
    k_gpio_init(K_LED_ORANGE, K_GPIO_OUTPUT, K_GPIO_PULL_NONE);
    k_gpio_init(K_LED_RED, K_GPIO_OUTPUT, K_GPIO_PULL_NONE);
    k_gpio_init(K_LED_BLUE, K_GPIO_OUTPUT, K_GPIO_PULL_NONE);
    k_gpio_init(K_BUTTON_0, K_GPIO_INPUT, K_GPIO_PULL_NONE);
    #endif

    #ifdef TARGET_LIKE_MSP430
    k_gpio_init(K_LED_GREEN, K_GPIO_OUTPUT, K_GPIO_PULL_NONE);
    k_gpio_init(K_LED_RED, K_GPIO_OUTPUT, K_GPIO_PULL_NONE);
    k_gpio_init(K_BUTTON_0, K_GPIO_INPUT, K_GPIO_PULL_UP);
    /* Stop the watchdog. */
    WDTCTL = WDTPW + WDTHOLD;

    __enable_interrupt();

    P2OUT = BIT1;
    #endif


    xTaskCreate(task_i2c, "I2C", configMINIMAL_STACK_SIZE * 2, NULL, 2, NULL);

    vTaskStartScheduler();

    while (1);

    return 0;
}
