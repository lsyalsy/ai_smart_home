#ifndef __INMP441_H
#define __INMP441_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* INMP441 I2S 数字麦克风引脚定义
 * BCLK (SCK) -> GPIO9
 * WS   (LRCK)-> GPIO10
 * DIN  (SD)  -> GPIO11
 * VDD -> 3.3V, GND -> GND, L/R -> GND (左声道)
 */
#define INMP441_BCLK_GPIO   9
#define INMP441_WS_GPIO     10
#define INMP441_DIN_GPIO    11

/* 音频参数：16kHz 单声道 16bit
 * INMP441 本身是 24bit，但 16bit 已足够用于 VAD/ASR 前端
 */
#define INMP441_SAMPLE_RATE 16000
#define INMP441_BITS        16

/**
 * @brief 初始化 I2S RX 通道，配置为飞利浦标准格式、16bit、16kHz 单声道。
 * @return true 初始化成功，false 失败
 */
bool inmp441_init(void);

/**
 * @brief 从 I2S 读取采样数据。
 *
 * @param buffer       接收缓冲区，大小至少为 sample_count * sizeof(int16_t)
 * @param sample_count 期望读取的采样点数（单声道样点数）
 *
 * @return 实际读取到的采样点数；出错或超时返回 0
 */
size_t inmp441_read(int16_t *buffer, size_t sample_count);

/**
 * @brief 反初始化 I2S RX 通道，释放资源。
 */
void inmp441_deinit(void);

#ifdef __cplusplus
}
#endif

#endif /* __INMP441_H */
