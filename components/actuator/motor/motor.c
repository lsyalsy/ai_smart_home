#include "motor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "MOTOR";

/* LEDC 配置：使用低速模式，25kHz 可避免风扇啸叫 */
#define MOTOR_LEDC_MODE         LEDC_LOW_SPEED_MODE
#define MOTOR_LEDC_TIMER        LEDC_TIMER_0
#define MOTOR_LEDC_FREQ_HZ      25000
#define MOTOR_LEDC_RESOLUTION   LEDC_TIMER_8_BIT
#define MOTOR_LEDC_MAX_DUTY     255

/* A 通道使用 LEDC_CHANNEL_0，B 通道使用 LEDC_CHANNEL_1 */
#define MOTOR_LEDC_CH_A         LEDC_CHANNEL_0
#define MOTOR_LEDC_CH_B         LEDC_CHANNEL_1

/* 每个通道的运行时状态 */
typedef struct {
    uint8_t speed_level;
    uint8_t last_nonzero_level;
    int8_t  in1_gpio;
    int8_t  in2_gpio;
    ledc_channel_t ledc_ch;
} motor_chan_state_t;

static motor_chan_state_t s_chan[MOTOR_CHANNEL_MAX] = {
    [MOTOR_CHANNEL_A] = {
        .speed_level = 0,
        .last_nonzero_level = MOTOR_SPEED_MAX_LEVEL,
        .in1_gpio = MOTOR_AIN1_GPIO,
        .in2_gpio = MOTOR_AIN2_GPIO,
        .ledc_ch  = MOTOR_LEDC_CH_A,
    },
    [MOTOR_CHANNEL_B] = {
        .speed_level = 0,
        .last_nonzero_level = MOTOR_SPEED_MAX_LEVEL,
        .in1_gpio = MOTOR_BIN1_GPIO,
        .in2_gpio = MOTOR_BIN2_GPIO,
        .ledc_ch  = MOTOR_LEDC_CH_B,
    },
};

/* 配置方向控制 GPIO */
static void motor_gpio_init(int8_t gpio)
{
    if (gpio < 0) return;
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << gpio),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
}

void motor_init(void)
{
    /* A 通道方向引脚 */
    motor_gpio_init(MOTOR_AIN1_GPIO);
    motor_gpio_init(MOTOR_AIN2_GPIO);
    /* B 通道方向引脚 */
    motor_gpio_init(MOTOR_BIN1_GPIO);
    motor_gpio_init(MOTOR_BIN2_GPIO);

    /* LEDC 定时器配置（A/B 共用同一定时器） */
    ledc_timer_config_t timer_conf = {
        .speed_mode       = MOTOR_LEDC_MODE,
        .duty_resolution  = MOTOR_LEDC_RESOLUTION,
        .timer_num        = MOTOR_LEDC_TIMER,
        .freq_hz          = MOTOR_LEDC_FREQ_HZ,
        .clk_cfg          = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer_conf));

    /* A 通道 PWM */
    ledc_channel_config_t ch_a = {
        .gpio_num   = MOTOR_PWMA_GPIO,
        .speed_mode = MOTOR_LEDC_MODE,
        .channel    = MOTOR_LEDC_CH_A,
        .timer_sel  = MOTOR_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_a));

    /* B 通道 PWM */
    ledc_channel_config_t ch_b = {
        .gpio_num   = MOTOR_PWMB_GPIO,
        .speed_mode = MOTOR_LEDC_MODE,
        .channel    = MOTOR_LEDC_CH_B,
        .timer_sel  = MOTOR_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
        .intr_type  = LEDC_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch_b));

    /* 默认正转方向：AIN1=1, AIN2=0 */
    gpio_set_level(MOTOR_AIN1_GPIO, 1);
    gpio_set_level(MOTOR_AIN2_GPIO, 0);
    gpio_set_level(MOTOR_BIN1_GPIO, 1);
    gpio_set_level(MOTOR_BIN2_GPIO, 0);

    ESP_LOGI(TAG, "TB6612 dual motor init: A(IN1=%d,IN2=%d,PWM=%d) B(IN1=%d,IN2=%d,PWM=%d)",
             MOTOR_AIN1_GPIO, MOTOR_AIN2_GPIO, MOTOR_PWMA_GPIO,
             MOTOR_BIN1_GPIO, MOTOR_BIN2_GPIO, MOTOR_PWMB_GPIO);
}

void motor_deinit(void)
{
    motor_off();
    motor_off_ch(MOTOR_CHANNEL_B);
    ledc_stop(MOTOR_LEDC_MODE, MOTOR_LEDC_CH_A, 0);
    ledc_stop(MOTOR_LEDC_MODE, MOTOR_LEDC_CH_B, 0);
}

void motor_set_speed_ch(motor_channel_t ch, uint8_t level)
{
    if (ch >= MOTOR_CHANNEL_MAX) return;
    if (level > MOTOR_SPEED_MAX_LEVEL) {
        level = MOTOR_SPEED_MAX_LEVEL;
    }

    motor_chan_state_t *cs = &s_chan[ch];
    cs->speed_level = level;
    if (level > 0) {
        cs->last_nonzero_level = level;
    }

    /* duty = level / 20 * 255 */
    uint32_t duty = ((uint32_t)level * MOTOR_LEDC_MAX_DUTY + MOTOR_SPEED_MAX_LEVEL / 2)
                    / MOTOR_SPEED_MAX_LEVEL;

    ledc_set_duty(MOTOR_LEDC_MODE, cs->ledc_ch, duty);
    ledc_update_duty(MOTOR_LEDC_MODE, cs->ledc_ch);

    ESP_LOGI(TAG, "ch%d set speed=%u, duty=%lu/%d (%u%%)",
             ch, level, (unsigned long)duty, MOTOR_LEDC_MAX_DUTY,
             (unsigned int)level * MOTOR_SPEED_STEP_PERCENT);
}

void motor_set_speed(uint8_t level)
{
    motor_set_speed_ch(MOTOR_CHANNEL_A, level);
}

uint8_t motor_get_speed_ch(motor_channel_t ch)
{
    if (ch >= MOTOR_CHANNEL_MAX) return 0;
    return s_chan[ch].speed_level;
}

uint8_t motor_get_speed(void)
{
    return motor_get_speed_ch(MOTOR_CHANNEL_A);
}

void motor_on_ch(motor_channel_t ch)
{
    if (ch >= MOTOR_CHANNEL_MAX) return;
    motor_chan_state_t *cs = &s_chan[ch];
    if (cs->speed_level == 0) {
        motor_set_speed_ch(ch, cs->last_nonzero_level > 0 ? cs->last_nonzero_level : MOTOR_SPEED_MAX_LEVEL);
    }
}

void motor_on(void)
{
    motor_on_ch(MOTOR_CHANNEL_A);
}

void motor_off_ch(motor_channel_t ch)
{
    motor_set_speed_ch(ch, 0);
}

void motor_off(void)
{
    motor_off_ch(MOTOR_CHANNEL_A);
}

void motor_toggle_ch(motor_channel_t ch)
{
    if (ch >= MOTOR_CHANNEL_MAX) return;
    if (s_chan[ch].speed_level > 0) {
        motor_off_ch(ch);
    } else {
        motor_on_ch(ch);
    }
}

void motor_toggle(void)
{
    motor_toggle_ch(MOTOR_CHANNEL_A);
}

bool motor_state_ch(motor_channel_t ch)
{
    if (ch >= MOTOR_CHANNEL_MAX) return false;
    return s_chan[ch].speed_level > 0;
}

bool motor_state(void)
{
    return motor_state_ch(MOTOR_CHANNEL_A);
}
