#include "bh1750.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

static const char *TAG = "bh1750";

static i2c_master_bus_handle_t  i2c_bus_handle   = NULL;
static i2c_master_dev_handle_t  bh1750_dev_handle = NULL;

void bh1750_init(void)
{
    if (i2c_bus_handle != NULL || bh1750_dev_handle != NULL) {
        ESP_LOGW(TAG, "BH1750 already initialized");
        return;
    }

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port   = I2C_NUM_0,
        .sda_io_num = BH1750_I2C_SDA_GPIO,
        .scl_io_num = BH1750_I2C_SCL_GPIO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_bus_config, &i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C master bus creation failed: %s", esp_err_to_name(ret));
        return;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = BH1750_ADDR,
        .scl_speed_hz    = 100000,
    };

    ret = i2c_master_bus_add_device(i2c_bus_handle, &dev_config, &bh1750_dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add BH1750 device: %s", esp_err_to_name(ret));
        return;
    }

    /* 上电 */
    uint8_t power_on = BH1750_CMD_POWER_ON;
    ret = i2c_master_transmit(bh1750_dev_handle, &power_on, 1, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750 power on failed: %s", esp_err_to_name(ret));
        return;
    }

    /* 设置为连续高分辨率模式 */
    uint8_t cont_h_res = BH1750_CMD_CONT_H_RES;
    ret = i2c_master_transmit(bh1750_dev_handle, &cont_h_res, 1, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750 set continuous H-res mode failed: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "BH1750 initialized, SDA=GPIO%d, SCL=GPIO%d, addr=0x%02X",
             BH1750_I2C_SDA_GPIO, BH1750_I2C_SCL_GPIO, BH1750_ADDR);
}

uint16_t bh1750_read_lux(void)
{
    if (bh1750_dev_handle == NULL) {
        ESP_LOGE(TAG, "BH1750 not initialized");
        return 0;
    }

    uint8_t data[2] = {0};
    esp_err_t ret = i2c_master_receive(bh1750_dev_handle, data, 2, -1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "BH1750 read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    uint16_t raw = ((uint16_t)data[0] << 8) | data[1];
    uint16_t lux = (uint16_t)(raw / 1.2f);

    return lux;
}
