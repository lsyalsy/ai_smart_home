#ifndef BLE_HR_H
#define BLE_HR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 初始化 NimBLE Host，启动 BLE 心率采集。
 * 会自动扫描并连接广播了心率服务 0x180D 的设备，
 * 订阅心率测量特征 0x2A37 的通知。
 */
void ble_hr_init(void);

/**
 * 获取最近一次收到的心率值（单位：bpm）。
 * 若未连接或尚未收到数据，返回 0。
 */
uint8_t ble_hr_get_heart_rate(void);

#ifdef __cplusplus
}
#endif

#endif /* BLE_HR_H */
