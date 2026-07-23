#ifndef __BH1750_H
#define __BH1750_H

#include <stdint.h>

/* BH1750 I2C 总线引脚定义 */
#define BH1750_I2C_SDA_GPIO     8
#define BH1750_I2C_SCL_GPIO     9

/* BH1750 I2C 设备地址 (ADDR 引脚接地) */
#define BH1750_ADDR             0x23

/* BH1750 指令 */
#define BH1750_CMD_POWER_DOWN   0x00
#define BH1750_CMD_POWER_ON     0x01
#define BH1750_CMD_RESET        0x07
#define BH1750_CMD_CONT_H_RES   0x10  /* 连续高分辨率模式 (1 lx) */
#define BH1750_CMD_CONT_H_RES2  0x11  /* 连续高分辨率模式2 (0.5 lx) */
#define BH1750_CMD_CONT_L_RES   0x13  /* 连续低分辨率模式 (4 lx) */

/**
 * @brief 初始化 BH1750 光照传感器
 *
 * 配置 I2C 主控总线 (SDA=GPIO5, SCL=GPIO6)，向 BH1750 发送上电和
 * 连续高分辨率模式指令。
 */
void bh1750_init(void);

/**
 * @brief 读取 BH1750 光照强度
 *
 * @return 光照强度值，单位为 lux
 */
uint16_t bh1750_read_lux(void);

#endif /* __BH1750_H */
