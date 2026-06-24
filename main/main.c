/*
 * AI 智能管家 - 硬件综合测试主程序
 * 读取真实传感器数据并在 TFT 上显示，同时自动切换执行器状态方便逐一测试。
 */
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#include "st7735.h"
#include "ui.h"

#include "led.h"
#include "buzzer.h"
#include "motor.h"
#include "relay.h"
#include "ec11.h"

#include "hcsr501.h"
#include "dht22.h"
#include "bh1750.h"
#include "mq2.h"
#include "ble_hr.h"

#define BTN_PAGE_GPIO   37   /* 页面切换按键 */
#define BTN_PRESS_LEVEL 0
#define BTN_POLL_MS     50
#define BTN_DEBOUNCE_MS 200

#define SENSOR_UPDATE_MS   2000   /* 每 2 秒刷新一次传感器 */
#define ACTUATOR_TEST_MS   5000   /* 每 5 秒自动切换执行器 */
#define MQ2_ALARM_THRESHOLD 2000  /* 烟雾报警阈值，根据实际模块调整 */

static void btn_page_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_PAGE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

static bool btn_page_pressed(void)
{
    if (gpio_get_level(BTN_PAGE_GPIO) == BTN_PRESS_LEVEL) {
        vTaskDelay(pdMS_TO_TICKS(BTN_DEBOUNCE_MS));
        if (gpio_get_level(BTN_PAGE_GPIO) == BTN_PRESS_LEVEL) {
            while (gpio_get_level(BTN_PAGE_GPIO) == BTN_PRESS_LEVEL) {
                vTaskDelay(pdMS_TO_TICKS(20));
            }
            vTaskDelay(pdMS_TO_TICKS(BTN_DEBOUNCE_MS));
            return true;
        }
    }
    return false;
}

static void hardware_init(void)
{
    printf("Initializing hardware...\n");

    /* 执行器 */
    led_init();
    buzzer_init();
    motor_init();
    relay_init();

    /* 输入设备 */
    ec11_init();

    /* 传感器 */
    hcsr501_init();
    dht22_init();
    bh1750_init();
    mq2_init();

    /* BLE 心率采集 */
    ble_hr_init();

    printf("Hardware init done.\n");
}

static void read_sensors(system_state_t *s)
{
    float temp = 0.0f, hum = 0.0f;
    if (dht22_read(&temp, &hum)) {
        s->temperature = temp;
        s->humidity    = hum;
    } else {
        printf("DHT22 read failed\n");
    }

    s->light_lx     = bh1750_read_lux();
    s->human_present = hcsr501_detected();
    s->smoke_alarm  = mq2_alarm(MQ2_ALARM_THRESHOLD);

    /* BLE 心率（未连接/无数据时返回 0） */
    s->heart_rate = ble_hr_get_heart_rate();
}

static void update_actuator_state(system_state_t *s)
{
    s->led_on        = led_state();
    s->fan_mode      = motor_state() ? FAN_MODE_MANUAL : FAN_MODE_OFF;
    s->humidifier_on = relay_state();
    s->alarm_triggered = s->smoke_alarm;
}

static void actuator_test_step(void)
{
    static int step = 0;
    switch (step) {
        case 0: led_toggle(); break;
        case 1: motor_toggle(); break;
        case 2: relay_toggle(); break;
        case 3: buzzer_beep(200); break;
    }
    step = (step + 1) % 4;
}

void app_main(void)
{
    printf("AI Smart Home hardware test starting...\n");

    tft_init();
    btn_page_init();
    hardware_init();

    system_state_t state = {0};
    state.heart_rate = 72;
    snprintf(state.suggestion, sizeof(state.suggestion),
             "当前环境舒适，注意保持通风");
    state.alarm_hour = 7;
    state.alarm_minute = 30;
    state.alarm_mode = ALARM_MODE_LIGHT;
    state.alarm_enabled = true;

    ui_page_t page = PAGE_DATA;
    ui_render_page(page, &state);

    int sensor_timer = 0;
    int actuator_timer = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(BTN_POLL_MS));
        sensor_timer += BTN_POLL_MS;
        actuator_timer += BTN_POLL_MS;

        /* EC11 旋转编码器输入 */
        int16_t delta = ec11_read_delta();
        if (delta != 0) {
            if (page == PAGE_ALARM) {
                int16_t new_minute = (int16_t)state.alarm_minute + delta;
                while (new_minute < 0) new_minute += 60;
                while (new_minute >= 60) new_minute -= 60;
                state.alarm_minute = (uint8_t)new_minute;
                printf("EC11 alarm minute: %d\n", state.alarm_minute);
                ui_update_page(&state);
            } else {
                /* 顺时针下一页，逆时针上一页，循环 */
                int16_t steps = delta;
                while (steps > 0) {
                    page++;
                    if (page >= PAGE_MAX) page = PAGE_DATA;
                    steps--;
                }
                while (steps < 0) {
                    if (page == PAGE_DATA) page = PAGE_MAX - 1;
                    else page--;
                    steps++;
                }
                printf("EC11 switch to page %d\n", page);
                ui_render_page(page, &state);
            }
            continue;
        }

        if (ec11_button_pressed()) {
            state.alarm_enabled = !state.alarm_enabled;
            printf("EC11 alarm enabled: %d\n", state.alarm_enabled);
            ui_update_page(&state);
            continue;
        }

        /* 页面切换按键（GPIO37，保留作为备用） */
        if (btn_page_pressed()) {
            page++;
            if (page >= PAGE_MAX) page = PAGE_DATA;
            printf("Switch to page %d\n", page);
            ui_render_page(page, &state);
            continue;
        }

        /* 定时读取传感器并刷新数据页/状态页（局部刷新，避免整屏闪烁） */
        if (sensor_timer >= SENSOR_UPDATE_MS) {
            sensor_timer = 0;
            read_sensors(&state);
            update_actuator_state(&state);
            printf("T=%.1f H=%.1f L=%d PIR=%d MQ2=%lu\n",
                   state.temperature, state.humidity, state.light_lx,
                   state.human_present, (unsigned long)mq2_read_raw());

            if (page == PAGE_DATA || page == PAGE_STATUS) {
                ui_update_page(&state);
            }
        }

        /* 定时自动切换执行器，方便逐一测试 */
        if (actuator_timer >= ACTUATOR_TEST_MS) {
            actuator_timer = 0;
            actuator_test_step();
            update_actuator_state(&state);
            if (page == PAGE_STATUS) {
                ui_update_page(&state);
            }
        }
    }
}
