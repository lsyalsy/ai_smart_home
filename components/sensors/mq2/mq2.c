#include "mq2.h"

#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

#define MQ2_TAG            "MQ2"
#define MQ2_ADC_UNIT       ADC_UNIT_1
#define MQ2_ADC_CHANNEL    ADC_CHANNEL_1  /* GPIO2 on ESP32-S3 */
#define MQ2_ADC_ATTEN      ADC_ATTEN_DB_12
#define MQ2_ADC_BITWIDTH   ADC_BITWIDTH_12
#define MQ2_AVG_SAMPLES    4

static adc_oneshot_unit_handle_t s_adc_handle = NULL;

void mq2_init(void)
{
    if (s_adc_handle != NULL) {
        ESP_LOGW(MQ2_TAG, "MQ-2 already initialized");
        return;
    }

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = MQ2_ADC_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &s_adc_handle));

    adc_oneshot_chan_cfg_t chan_config = {
        .atten = MQ2_ADC_ATTEN,
        .bitwidth = MQ2_ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc_handle, MQ2_ADC_CHANNEL, &chan_config));

    ESP_LOGI(MQ2_TAG, "MQ-2 initialized on ADC1 channel 1 (GPIO2)");
}

uint32_t mq2_read_raw(void)
{
    int raw_value = 0;

    if (s_adc_handle == NULL) {
        ESP_LOGE(MQ2_TAG, "MQ-2 not initialized, call mq2_init() first");
        return 0;
    }

    /* 滑动平均滤波：连续采样多次取平均，减少跳变 */
    int sum = 0;
    for (int i = 0; i < MQ2_AVG_SAMPLES; i++) {
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc_handle, MQ2_ADC_CHANNEL, &raw_value));
        sum += raw_value;
    }
    return (uint32_t)(sum / MQ2_AVG_SAMPLES);
}

bool mq2_alarm(uint32_t threshold)
{
    uint32_t raw = mq2_read_raw();
    return raw > threshold;
}
