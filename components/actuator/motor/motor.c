#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

/* LEDC 配置：使用低速模式，25kHz 可避免风扇啸叫 */
#define MOTOR_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define MOTOR_LEDC_CHANNEL      LEDC_CHANNEL_0
#define MOTOR_LEDC_TIMER        LEDC_TIMER_0
#define MOTOR_LEDC_FREQ_HZ      25000
#define MOTOR_LEDC_RESOLUTION   LEDC_TIMER_8_BIT
#define MOTOR_LEDC_MAX_DUTY     255

static uint8_t s_speed_level = 0;
static uint8_t s_last_nonzero_level = MOTOR_SPEED_MAX_LEVEL;

void motor_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << MOTOR_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    ledc_timer_config_t timer_conf = {
        .speed_mode       = MOTOR_LEDC_MODE,
        .duty_resolution  = MOTOR_LEDC_RESOLUTION,
        .timer_num        = MOTOR_LEDC_TIMER,
        .freq_hz          = MOTOR_LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    ledc_channel_config_t ch_conf = {
        .gpio_num       = MOTOR_GPIO,
        .speed_mode     = MOTOR_LEDC_MODE,
        .channel        = MOTOR_LEDC_CHANNEL,
        .timer_sel      = MOTOR_LEDC_TIMER,
        .duty           = 0,
        .hpoint         = 0,
        .intr_type      = LEDC_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_conf));

    s_speed_level = 0;
    s_last_nonzero_level = MOTOR_SPEED_MAX_LEVEL;

    ESP_LOGI(TAG, "motor PWM init ok: GPIO%d, %dHz, %d levels",
             MOTOR_GPIO, MOTOR_LEDC_FREQ_HZ, MOTOR_SPEED_MAX_LEVEL);
}

void motor_deinit(void)
{
    motor_off();
    ledc_stop(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL, 0);
}

void motor_set_speed(uint8_t level)
{
    if (level > MOTOR_SPEED_MAX_LEVEL) {
        level = MOTOR_SPEED_MAX_LEVEL;
    }

    s_speed_level = level;
    if (level > 0) {
        s_last_nonzero_level = level;
    }

    /* duty = level / 20 * 255，四舍五入 */
    uint32_t duty = ((uint32_t)level * MOTOR_LEDC_MAX_DUTY + MOTOR_SPEED_MAX_LEVEL / 2)
                    / MOTOR_SPEED_MAX_LEVEL;

    ledc_set_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL, duty);
    ledc_update_duty(MOTOR_LEDC_MODE, MOTOR_LEDC_CHANNEL);

    ESP_LOGI(TAG, "set speed level=%u, duty=%lu/%d (%u%%)",
             level, (unsigned long)duty, MOTOR_LEDC_MAX_DUTY,
             (unsigned int)level * MOTOR_SPEED_STEP_PERCENT);
}

uint8_t motor_get_speed(void)
{
    return s_speed_level;
}

void motor_on(void)
{
    if (s_speed_level == 0) {
        motor_set_speed(s_last_nonzero_level > 0 ? s_last_nonzero_level : MOTOR_SPEED_MAX_LEVEL);
    }
}

void motor_off(void)
{
    motor_set_speed(0);
}

void motor_toggle(void)
{
    if (s_speed_level > 0) {
        motor_off();
    } else {
        motor_on();
    }
}

bool motor_state(void)
{
    return s_speed_level > 0;
}
