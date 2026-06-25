#include "voice.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized   = false;
static uint32_t     s_vad_threshold = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state     = VAD_STATE_SILENCE;
static uint32_t     s_hangover      = 0;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）。
 */
static uint32_t voice_calc_energy(const int16_t *buffer, size_t sample_cnt)
{
    if (buffer == NULL || sample_cnt == 0) {
        return 0;
    }

    int64_t sum = 0;
    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        sum += (v >= 0 ? v : -v);
    }

    return (uint32_t)(sum / (int64_t)sample_cnt);
}

bool voice_init(void)
{
    if (s_initialized) {
        return true;
    }

    if (!inmp441_init()) {
        ESP_LOGE(TAG, "INMP441 init failed");
        return false;
    }

    memset(s_pcm_buffer, 0, sizeof(s_pcm_buffer));
    s_vad_state     = VAD_STATE_SILENCE;
    s_vad_threshold = VAD_ENERGY_DEFAULT;
    s_hangover      = 0;
    s_initialized   = true;

    ESP_LOGI(TAG, "voice init ok, VAD threshold=%lu, frame=%d samples",
             (unsigned long)s_vad_threshold, VAD_SAMPLE_COUNT);
    return true;
}

vad_state_t voice_process(void)
{
    if (!s_initialized) {
        return VAD_STATE_SILENCE;
    }

    size_t read_cnt = inmp441_read(s_pcm_buffer, VAD_SAMPLE_COUNT);
    if (read_cnt == 0) {
        return s_vad_state;
    }

    uint32_t energy = voice_calc_energy(s_pcm_buffer, read_cnt);
    bool is_active  = (energy >= s_vad_threshold);

    if (is_active) {
        s_hangover = VAD_HANGOVER_FRAMES;
        if (s_vad_state != VAD_STATE_ACTIVE) {
            s_vad_state = VAD_STATE_ACTIVE;
            ESP_LOGI(TAG, "VAD ACTIVE: energy=%lu", (unsigned long)energy);
        }
    } else {
        if (s_hangover > 0) {
            s_hangover--;
        }
        if (s_hangover == 0 && s_vad_state != VAD_STATE_SILENCE) {
            s_vad_state = VAD_STATE_SILENCE;
            ESP_LOGI(TAG, "VAD SILENCE");
        }
    }

    /* TODO: 检测到语音活动后，可将原始音频缓存并送入 ASR */
    if (s_vad_state == VAD_STATE_ACTIVE) {
        /* 预留 ASR 接入点：voice_asr_send(s_pcm_buffer, read_cnt); */
    }

    return s_vad_state;
}

void voice_set_vad_threshold(uint32_t threshold)
{
    s_vad_threshold = threshold;
    ESP_LOGI(TAG, "VAD threshold set to %lu", (unsigned long)threshold);
}

/* ==================== 预留接口：后续接入云端 ASR/LLM/TTS ==================== */

void voice_asr_send(const int16_t *buffer, size_t sample_cnt)
{
    ESP_LOGI(TAG, "ASR send stub: %u samples", (unsigned int)sample_cnt);
    (void)buffer;
}

void voice_llm_query(const char *text)
{
    ESP_LOGI(TAG, "LLM query stub: %s", text ? text : "(null)");
}

void voice_tts_play(const char *text)
{
    ESP_LOGI(TAG, "TTS play stub: %s", text ? text : "(null)");
}
#include "voice#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/*#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define V#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_EN#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  =#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0)#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i =#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs_v;
        }
    }

    *out_energy = (uint32_t)(#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs_v;
        }
    }

    *out_energy = (uint32_t)(sum / (int64_t)sample_cnt);
    *out_peak   = peak;
#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs_v;
        }
    }

    *out_energy = (uint32_t)(sum / (int64_t)sample_cnt);
    *out_peak   = peak;
}

bool voice_init(void)
{
    if (s_initialized) {
#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs_v;
        }
    }

    *out_energy = (uint32_t)(sum / (int64_t)sample_cnt);
    *out_peak   = peak;
}

bool voice_init(void)
{
    if (s_initialized) {
        return true;
    }

    if#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs_v;
        }
    }

    *out_energy = (uint32_t)(sum / (int64_t)sample_cnt);
    *out_peak   = peak;
}

bool voice_init(void)
{
    if (s_initialized) {
        return true;
    }

    if (!inmp441_init()) {
        ESP_LOGE(TAG, "INMP441 init failed#include "voice.h"

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "esp_timer.h"

#include "inmp441.h"

static const char *TAG = "VOICE";

/* 每帧处理 100ms @ 16kHz 单声道 = 1600 个采样 */
#define VAD_FRAME_MS        100
#define VAD_SAMPLE_COUNT    ((INMP441_SAMPLE_RATE / 1000) * VAD_FRAME_MS)

/* VAD 默认能量阈值与挂起帧数
 * 能量 = 采样绝对值平均值；环境安静时通常 < 100，说话声 > 500
 */
#define VAD_ENERGY_DEFAULT  500
#define VAD_HANGOVER_FRAMES 5

/* 模块状态 */
static bool         s_initialized    = false;
static uint32_t     s_vad_threshold  = VAD_ENERGY_DEFAULT;
static vad_state_t  s_vad_state      = VAD_STATE_SILENCE;
static uint32_t     s_hangover       = 0;
static bool         s_print_energy   = true;

/* 音频采样缓冲区 */
static int16_t s_pcm_buffer[VAD_SAMPLE_COUNT];

/* 每秒统计 */
static uint32_t s_frame_energy_sum  = 0;
static uint32_t s_frame_energy_max  = 0;
static uint32_t s_frame_energy_peak = 0;
static uint32_t s_frame_count       = 0;
static int64_t  s_stats_last_us     = 0;

/**
 * @brief 计算一帧音频的平均能量（绝对值平均）和最大幅值。
 */
static void voice_calc_stats(const int16_t *buffer, size_t sample_cnt,
                             uint32_t *out_energy, uint32_t *out_peak)
{
    *out_energy = 0;
    *out_peak   = 0;

    if (buffer == NULL || sample_cnt == 0) {
        return;
    }

    int64_t sum  = 0;
    uint32_t peak = 0;

    for (size_t i = 0; i < sample_cnt; i++) {
        int32_t v = (int32_t)buffer[i];
        uint32_t abs_v = (uint32_t)(v >= 0 ? v : -v);
        sum += abs_v;
        if (abs_v > peak) {
            peak = abs_v;
        }
    }

    *out_energy = (uint32_t)(sum / (int64_t)sample_cnt);
    *out_peak   = peak;
}

bool voice_init(void)
{
    if (s_initialized) {
        return true;
    }

    if (!inmp441_init()) {
        ESP_LOGE(TAG, "INMP441 init failed");
        return false;
    }

    memset(s_pcm_buffer, 0