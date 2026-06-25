#include "inmp441.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "driver/i2s_std.h"

static const char *TAG = "INMP441";

/* I2S RX 通道句柄 */
static i2s_chan_handle_t s_rx_chan = NULL;

bool inmp441_init(void)
{
    if (s_rx_chan != NULL) {
        ESP_LOGW(TAG, "already initialized");
        return true;
    }

    /* 1. 创建 I2S 通道：只使用 RX，自动选择 I2S 端口，主机模式 */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = 1024;          /* 每帧 1024 个采样，降低中断频率 */
    chan_cfg.dma_desc_num  = 6;             /* DMA 描述符数量 */
    chan_cfg.auto_clear    = true;          /* 超时自动清零 FIFO，避免爆音 */

    esp_err_t ret = i2s_new_channel(&chan_cfg, NULL, &s_rx_chan);
    if (ret != ESP_OK || s_rx_chan == NULL) {
        ESP_LOGE(TAG, "i2s_new_channel failed: %s", esp_err_to_name(ret));
        return false;
    }

    /* 2. 标准飞利浦 I2S 配置：16kHz、16bit、单声道 */
    i2s_std_config_t std_cfg = {
        .clk_cfg   = I2S_STD_CLK_DEFAULT_CONFIG(INMP441_SAMPLE_RATE),
        .slot_cfg  = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,
                                                        I2S_SLOT_MODE_MONO),
        .gpio_cfg  = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = (gpio_num_t)INMP441_BCLK_GPIO,
            .ws   = (gpio_num_t)INMP441_WS_GPIO,
            .dout = I2S_GPIO_UNUSED,
            .din  = (gpio_num_t)INMP441_DIN_GPIO,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv   = false,
            },
        },
    };

    /* INMP441 L/R 接地 -> 左声道有效；单声道默认使用左声道 */
    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    ret = i2s_channel_init_std_mode(s_rx_chan, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_init_std_mode failed: %s", esp_err_to_name(ret));
        i2s_del_channel(s_rx_chan);
        s_rx_chan = NULL;
        return false;
    }

    /* 3. 启用通道 */
    ret = i2s_channel_enable(s_rx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2s_channel_enable failed: %s", esp_err_to_name(ret));
        i2s_del_channel(s_rx_chan);
        s_rx_chan = NULL;
        return false;
    }

    ESP_LOGI(TAG, "init ok: BCLK=GPIO%d WS=GPIO%d DIN=GPIO%d %dHz %dbit mono",
             INMP441_BCLK_GPIO, INMP441_WS_GPIO, INMP441_DIN_GPIO,
             INMP441_SAMPLE_RATE, INMP441_BITS);
    return true;
}

size_t inmp441_read(int16_t *buffer, size_t sample_count)
{
    if (s_rx_chan == NULL || buffer == NULL || sample_count == 0) {
        return 0;
    }

    size_t bytes_to_read = sample_count * sizeof(int16_t);
    size_t bytes_read    = 0;

    esp_err_t ret = i2s_channel_read(s_rx_chan, buffer, bytes_to_read,
                                     &bytes_read, pdMS_TO_TICKS(100));
    if (ret != ESP_OK && ret != ESP_ERR_TIMEOUT) {
        ESP_LOGE(TAG, "i2s_channel_read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    return bytes_read / sizeof(int16_t);
}

void inmp441_deinit(void)
{
    if (s_rx_chan == NULL) {
        return;
    }

    i2s_channel_disable(s_rx_chan);
    i2s_del_channel(s_rx_chan);
    s_rx_chan = NULL;

    ESP_LOGI(TAG, "deinit ok");
}
