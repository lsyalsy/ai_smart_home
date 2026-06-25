#ifndef __VOICE_H
#define __VOICE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* VAD 状态 */
typedef enum {
    VAD_STATE_SILENCE = 0,  /* 静音/无语音 */
    VAD_STATE_ACTIVE,       /* 检测到语音活动 */
} vad_state_t;

/**
 * @brief 初始化语音处理模块（含 INMP441 麦克风和 VAD）。
 * @return true 成功，false 失败
 */
bool voice_init(void);

/**
 * @brief 处理一帧音频：读取 INMP441 数据并做 VAD 检测。
 *
 * 该函数应在 task_voice 中按固定周期（如 100ms）调用。
 *
 * @return 当前 VAD 状态
 */
vad_state_t voice_process(void);

/**
 * @brief 动态设置 VAD 能量阈值。
 * @param threshold 能量阈值（16bit 采样绝对值平均），默认 500
 */
void voice_set_vad_threshold(uint32_t threshold);

/**
 * @brief 开关每秒一次的音频能量调试输出。
 * @param enable true 开启，false 关闭
 */
void voice_set_energy_print(bool enable);

/**
 * @brief 预留：将音频数据发送给 ASR 服务识别文本。
 * @param buffer     音频采样缓冲区
 * @param sample_cnt 采样点数
 */
void voice_asr_send(const int16_t *buffer, size_t sample_cnt);

/**
 * @brief 预留：将识别文本发送给大模型获取回答。
 * @param text 识别后的文本
 */
void voice_llm_query(const char *text);

/**
 * @brief 预留：播放 TTS 语音回复。
 * @param text 待播报文本
 */
void voice_tts_play(const char *text);

#ifdef __cplusplus
}
#endif

#endif /* __VOICE_H */
