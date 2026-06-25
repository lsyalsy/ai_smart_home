/*
 * AI 智能管家 - FreeRTOS 多任务重构版
 *
 * 任务划分：
 *   task_sensor : 每 500ms 读取传感器，更新全局 system_state_t
 *   task_display: 事件驱动，处理输入事件与显示刷新请求
 *   task_rules  : 每 500ms 根据规则自动控制执行器
 *   task_input  : 每 20ms 轮询 EC11 编码器与 GPIO37 按键
 *   task_voice  : 每 100ms 读取 INMP441 并做 VAD 检测
 */
#include <stdio.h>
#include <string.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

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

#include "voice.h"

#define TAG "APP"

/* ========== 引脚与定时常量 ========== */
#define BTN_PAGE_GPIO       37   /* 页面切换按键 */
#define BTN_PRESS_LEVEL     0
#define BTN_DEBOUNCE_MS     200

#define SENSOR_PERIOD_MS    500
#define RULES_PERIOD_MS     500
#define INPUT_PERIOD_MS     20
#define VOICE_PERIOD_MS     100

#define MQ2_ALARM_THRESHOLD 2000  /* 烟雾报警阈值 */

/* ========== 全局状态与同步原语 ========== */
system_state_t    g_state      = {0};
system_state_t    g_prev_state = {0}; /* 供 display 任务做按字段刷新 */
SemaphoreHandle_t g_state_mutex = NULL;

/* 输入事件队列（task_input -> task_display） */
QueueHandle_t g_input_queue = NULL;

/* 显示事件队列（task_sensor/task_rules -> task_display） */
QueueHandle_t g_display_queue = NULL;

/* QueueSet，让 task_display 同时监听两个队列 */
QueueSetHandle_t g_display_queue_set = NULL;

/* ========== 事件定义 ========== */
typedef enum {
    INPUT_ENCODER_DELTA = 0,  /* EC11 旋转 */
    INPUT_ENCODER_BTN,        /* EC11 按键 */
    INPUT_GPIO37_BTN,         /* GPIO37 页面按键 */
} input_event_type_t;

typedef struct {
    input_event_type_t type;
    int16_t            value; /* 旋转增量等 */
} input_event_t;

typedef enum {
    DISPLAY_REFRESH = 0,      /* 按字段刷新当前页面 */
    DISPLAY_PAGE_CHANGE,      /* 整屏切换页面 */
} display_event_type_t;

typedef struct {
    display_event_type_t type;
    ui_page_t            page; /* DISPLAY_PAGE_CHANGE 时有效 */
} display_event_t;

/* ========== 按键初始化与轮询 ========== */
static void btn_page_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << BTN_PAGE_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
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

/* ========== 硬件初始化 ========== */
static void app_hardware_init(void)
{
    ESP_LOGI(TAG, "Initializing hardware...");

    /* 执行器 */
    led_init();
    buzzer_init();
    motor_init();
    relay_init();

    /* 输入设备 */
    ec11_init();
    btn_page_init();

    /* 传感器 */
    hcsr501_init();
    dht22_init();
    bh1750_init();
    mq2_init();

    /* BLE 心率采集 */
    ble_hr_init();

    /* 语音（含 INMP441） */
    if (!voice_init()) {
        ESP_LOGE(TAG, "Voice init failed, voice task will be disabled");
    }

    ESP_LOGI(TAG, "Hardware init done.");
}

/* ========== 传感器读取 ========== */
static void read_sensors(system_state_t *s)
{
    float temp = 0.0f, hum = 0.0f;
    if (dht22_read(&temp, &hum)) {
        s->temperature = temp;
        s->humidity    = hum;
    } else {
        ESP_LOGW(TAG, "DHT22 read failed");
    }

    s->light_lx      = bh1750_read_lux();
    s->human_present = hcsr501_detected();
    s->smoke_alarm   = mq2_alarm(MQ2_ALARM_THRESHOLD);

    /* BLE 心率（未连接/无数据时返回 0） */
    s->heart_rate = ble_hr_get_heart_rate();
}

/* ========== FreeRTOS 任务 ========== */

/* sensor 任务：每 500ms 读取一次传感器 */
static void task_sensor(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "task_sensor start");

    const TickType_t period = pdMS_TO_TICKS(SENSOR_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, period);

        xSemaphoreTake(g_state_mutex, portMAX_DELAY);
        read_sensors(&g_state);
        xSemaphoreGive(g_state_mutex);

        ESP_LOGI(TAG, "Sensor T=%.1f H=%.1f L=%d PIR=%d MQ2=%lu HR=%u",
                 g_state.temperature, g_state.humidity, g_state.light_lx,
                 g_state.human_present, (unsigned long)mq2_read_raw(),
                 g_state.heart_rate);

        display_event_t ev = {
            .type = DISPLAY_REFRESH,
            .page = PAGE_DATA
        };
        xQueueSend(g_display_queue, &ev, 0);
    }
}

/* rules 任务：每 500ms 根据规则控制执行器 */
static void task_rules(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "task_rules start");

    const TickType_t period = pdMS_TO_TICKS(RULES_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, period);

        xSemaphoreTake(g_state_mutex, portMAX_DELAY);

        /* 规则1：自动模式下根据温度计算风扇档位
         * 温度 <= 28℃ 时关闭；>28℃ 后每升高 1℃ 增加一档，最高 20 档
         */
        if (g_state.fan_mode == FAN_MODE_AUTO) {
            float temp = g_state.temperature;
            uint8_t level = 0;
            if (temp > 28.0f) {
                level = (uint8_t)((temp - 28.0f) + 0.5f);
                if (level < 1) level = 1;
                if (level > MOTOR_SPEED_MAX_LEVEL) level = MOTOR_SPEED_MAX_LEVEL;
            }
            motor_set_speed(level);
            g_state.fan_speed_level = level;
        }

        /* 规则2：湿度 < 40% 开加湿器（继电器）
         * 烟雾报警时强制继电器打开，优先级更高 */
        if (g_state.smoke_alarm) {
            relay_on();
            g_state.humidifier_on = true;
        } else if (g_state.humidity < 40.0f) {
            relay_on();
            g_state.humidifier_on = true;
        } else {
            relay_off();
            g_state.humidifier_on = false;
        }

        /* 规则3：烟雾报警 -> 蜂鸣器 + LED + 继电器 */
        if (g_state.smoke_alarm) {
            buzzer_on();
            led_on();
            g_state.alarm_triggered = true;
            g_state.led_on = true;
        } else {
            buzzer_off();
            g_state.alarm_triggered = false;
            /* LED 状态在非报警时由其他逻辑决定，这里同步硬件状态 */
            g_state.led_on = led_state();
        }

        xSemaphoreGive(g_state_mutex);

        display_event_t ev = {
            .type = DISPLAY_REFRESH,
            .page = PAGE_DATA
        };
        xQueueSend(g_display_queue, &ev, 0);
    }
}

/* input 任务：每 20ms 轮询 EC11 与 GPIO37 按键 */
static void task_input(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "task_input start");

    const TickType_t period = pdMS_TO_TICKS(INPUT_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, period);

        int16_t delta = ec11_read_delta();
        if (delta != 0) {
            input_event_t ev = {
                .type  = INPUT_ENCODER_DELTA,
                .value = delta
            };
            xQueueSend(g_input_queue, &ev, 0);
        }

        if (ec11_button_pressed()) {
            input_event_t ev = {
                .type  = INPUT_ENCODER_BTN,
                .value = 0
            };
            xQueueSend(g_input_queue, &ev, 0);
        }

        if (btn_page_pressed()) {
            input_event_t ev = {
                .type  = INPUT_GPIO37_BTN,
                .value = 0
            };
            xQueueSend(g_input_queue, &ev, 0);
        }
    }
}

/* 处理输入事件，更新页面/闹钟状态；调用者已持有或无需持有 mutex */
static void handle_input_event(const input_event_t *ev, system_state_t *local_prev)
{
    xSemaphoreTake(g_state_mutex, portMAX_DELAY);

    ui_page_t page = ui_get_current_page();
    system_state_t current = g_state;

    switch (ev->type) {
        case INPUT_ENCODER_DELTA:
            /* EC11 旋转专职风扇调速（20 档，5% 步进） */
            {
                int16_t new_level = (int16_t)current.fan_speed_level + ev->value;
                if (new_level < 0) new_level = 0;
                if (new_level > MOTOR_SPEED_MAX_LEVEL) new_level = MOTOR_SPEED_MAX_LEVEL;
                current.fan_speed_level = (uint8_t)new_level;
                current.fan_mode = FAN_MODE_MANUAL;
                g_state.fan_speed_level = current.fan_speed_level;
                g_state.fan_mode = FAN_MODE_MANUAL;
                motor_set_speed(current.fan_speed_level);

                xSemaphoreGive(g_state_mutex);
                ui_update_page_state(local_prev, &current);
                *local_prev = current;
                ESP_LOGI(TAG, "EC11 fan speed: level=%d", current.fan_speed_level);
                return;
            }

        case INPUT_ENCODER_BTN:
            /* EC11 按键专职风扇开关 */
            motor_toggle();
            current.fan_speed_level = motor_get_speed();
            current.fan_mode = (current.fan_speed_level > 0) ? FAN_MODE_MANUAL : FAN_MODE_OFF;
            g_state.fan_speed_level = current.fan_speed_level;
            g_state.fan_mode = current.fan_mode;

            xSemaphoreGive(g_state_mutex);
            ui_update_page_state(local_prev, &current);
            *local_prev = current;
            ESP_LOGI(TAG, "EC11 fan toggle: %s, level=%d",
                     current.fan_speed_level > 0 ? "on" : "off",
                     current.fan_speed_level);
            return;

        case INPUT_GPIO37_BTN:
            page++;
            if (page >= PAGE_MAX) page = PAGE_DATA;

            xSemaphoreGive(g_state_mutex);
            ui_render_page(page, &current);
            *local_prev = current;
            ESP_LOGI(TAG, "GPIO37 switch to page %d", page);
            return;

        default:
            break;
    }

    xSemaphoreGive(g_state_mutex);
}

/* display 任务：事件驱动刷新 TFT */
static void task_display(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "task_display start");

    /* 保存上一次渲染状态，用于按字段刷新 */
    system_state_t local_prev = g_prev_state;

    while (1) {
        QueueSetMemberHandle_t member = xQueueSelectFromSet(g_display_queue_set, portMAX_DELAY);

        if (member == (QueueSetMemberHandle_t)g_input_queue) {
            input_event_t ev;
            if (xQueueReceive(g_input_queue, &ev, 0) == pdTRUE) {
                handle_input_event(&ev, &local_prev);
            }
        } else if (member == (QueueSetMemberHandle_t)g_display_queue) {
            display_event_t ev;
            if (xQueueReceive(g_display_queue, &ev, 0) == pdTRUE) {
                if (ev.type == DISPLAY_REFRESH) {
                    system_state_t current;
                    xSemaphoreTake(g_state_mutex, portMAX_DELAY);
                    current = g_state;
                    xSemaphoreGive(g_state_mutex);

                    ui_update_page_state(&local_prev, &current);
                    local_prev = current;
                } else if (ev.type == DISPLAY_PAGE_CHANGE) {
                    ui_render_page(ev.page, &g_state);
                    local_prev = g_state;
                }
            }
        }
    }
}

/* voice 任务：每 100ms 读取 INMP441 并做 VAD */
static void task_voice(void *pvParameter)
{
    (void)pvParameter;
    ESP_LOGI(TAG, "task_voice start");

    const TickType_t period = pdMS_TO_TICKS(VOICE_PERIOD_MS);
    TickType_t last_wake = xTaskGetTickCount();

    while (1) {
        vTaskDelayUntil(&last_wake, period);
        voice_process();
    }
}

/* ========== 同步原语与任务创建 ========== */
static void app_sync_init(void)
{
    g_state_mutex = xSemaphoreCreateMutex();
    configASSERT(g_state_mutex != NULL);

    g_input_queue = xQueueCreate(10, sizeof(input_event_t));
    configASSERT(g_input_queue != NULL);

    g_display_queue = xQueueCreate(10, sizeof(display_event_t));
    configASSERT(g_display_queue != NULL);

    /* QueueSet 容量必须 >= 两个队列深度之和 */
    g_display_queue_set = xQueueCreateSet(20);
    configASSERT(g_display_queue_set != NULL);

    xQueueAddToSet(g_input_queue, g_display_queue_set);
    xQueueAddToSet(g_display_queue, g_display_queue_set);
}

static void app_tasks_create(void)
{
    /* 堆栈与优先级说明：
     * task_input  : 4096 bytes, priority 7, 周期 20ms   （高响应输入）
     * task_display: 4096 bytes, priority 6, 事件驱动     （刷新 UI）
     * task_sensor : 4096 bytes, priority 5, 周期 500ms   （读传感器）
     * task_rules  : 4096 bytes, priority 5, 周期 500ms   （规则控制）
     * task_voice  : 8192 bytes, priority 5, 周期 100ms   （I2S + VAD）
     */
    xTaskCreatePinnedToCore(task_input,   "task_input",   4096, NULL, 7, NULL, 1);
    xTaskCreatePinnedToCore(task_display, "task_display", 4096, NULL, 6, NULL, 1);
    xTaskCreatePinnedToCore(task_sensor,  "task_sensor",  4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_rules,   "task_rules",   4096, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(task_voice,   "task_voice",   8192, NULL, 5, NULL, 0);
}

/* ========== app_main ========== */
void app_main(void)
{
    ESP_LOGI(TAG, "AI Smart Home starting...");

    /* 初始化全局状态默认值 */
    memset(&g_state, 0, sizeof(g_state));
    g_state.heart_rate  = 72;
    g_state.alarm_hour  = 7;
    g_state.alarm_minute = 30;
    g_state.alarm_mode  = ALARM_MODE_LIGHT;
    g_state.alarm_enabled = true;
    snprintf(g_state.suggestion, sizeof(g_state.suggestion),
             "当前环境舒适，注意保持通风");

    g_prev_state = g_state;

    /* TFT 初始化 + 首屏渲染（在创建任务前完成，避免任务竞争） */
    tft_init();
    ui_render_page(PAGE_DATA, &g_state);

    /* 同步原语 */
    app_sync_init();

    /* 硬件初始化（含 INMP441 / voice） */
    app_hardware_init();

    /* 创建 FreeRTOS 任务 */
    app_tasks_create();

    ESP_LOGI(TAG, "All tasks created, app_main done.");
}
