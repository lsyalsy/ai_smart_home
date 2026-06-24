#include "ec11.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "EC11"

/* EC11 机械抖动消抖时间：旋转 5ms，按键 50ms */
#define EC11_ROT_DEBOUNCE_US   5000
#define EC11_BTN_DEBOUNCE_US  50000

static volatile int16_t s_delta = 0;
static volatile bool    s_btn_pressed = false;
static volatile int64_t s_last_clk_us = 0;
static volatile int64_t s_last_btn_us = 0;
static portMUX_TYPE s_spinlock = portMUX_INITIALIZER_UNLOCKED;

/* CLK(A相) 下降沿触发：根据 B相(DT) 电平判断方向
 * 顺时针：A 下降时 B 为高
 * 逆时针：A 下降时 B 为低
 */
static void IRAM_ATTR ec11_clk_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();
    if ((now - s_last_clk_us) < EC11_ROT_DEBOUNCE_US) {
        return;
    }
    s_last_clk_us = now;

    int dt_level = gpio_get_level(EC11_DT_GPIO);
    if (dt_level == 1) {
        s_delta++;
    } else {
        s_delta--;
    }
}

/* SW 按键下降沿触发（按键按下为低电平） */
static void IRAM_ATTR ec11_sw_isr_handler(void *arg)
{
    int64_t now = esp_timer_get_time();
    if ((now - s_last_btn_us) < EC11_BTN_DEBOUNCE_US) {
        return;
    }
    s_last_btn_us = now;
    s_btn_pressed = true;
}

void ec11_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << EC11_CLK_GPIO) | (1ULL << EC11_DT_GPIO) | (1ULL << EC11_SW_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));

    ESP_ERROR_CHECK(gpio_set_intr_type(EC11_CLK_GPIO, GPIO_INTR_NEGEDGE));
    ESP_ERROR_CHECK(gpio_set_intr_type(EC11_SW_GPIO, GPIO_INTR_NEGEDGE));

    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_CLK_GPIO, ec11_clk_isr_handler, NULL));
    ESP_ERROR_CHECK(gpio_isr_handler_add(EC11_SW_GPIO, ec11_sw_isr_handler, NULL));

    ESP_LOGI(TAG, "EC11 init ok: CLK=GPIO%d DT=GPIO%d SW=GPIO%d",
             EC11_CLK_GPIO, EC11_DT_GPIO, EC11_SW_GPIO);
}

int16_t ec11_read_delta(void)
{
    portENTER_CRITICAL(&s_spinlock);
    int16_t delta = s_delta;
    s_delta = 0;
    portEXIT_CRITICAL(&s_spinlock);
    return delta;
}

bool ec11_button_pressed(void)
{
    portENTER_CRITICAL(&s_spinlock);
    bool pressed = s_btn_pressed;
    s_btn_pressed = false;
    portEXIT_CRITICAL(&s_spinlock);
    return pressed;
}
