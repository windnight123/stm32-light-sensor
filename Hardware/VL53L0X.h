#ifndef __VL53L0X_H
#define __VL53L0X_H

#include <stdint.h>

/*==========================================
 * VL53L0X 激光测距传感器
 *
 * 引脚连接 (与 OLED/BH1750 共用 I2C 总线):
 *   VL53L0X SCL   ->  PB6  (与 OLED/BH1750 SCL 并联)
 *   VL53L0X SDA   ->  PB7  (与 OLED/BH1750 SDA 并联)
 *   VL53L0X VCC   ->  3.3V
 *   VL53L0X GND   ->  GND
 *   VL53L0X XSHUT ->  VCC 或悬空 (模块内部已上拉)
 *
 * I2C 总线汇总:
 *   PB6 (SCL): OLED + BH1750 + VL53L0X
 *   PB7 (SDA): OLED + BH1750 + VL53L0X
 *==========================================*/

/* I2C 地址: 0x29 (7-bit) -> 0x52 (8-bit 写) */
#define VL53L0X_ADDR_DEFAULT  0x29

/**
  * @brief  初始化 VL53L0X (GPIO, 传感器初始化, 启动连续测距)
  * @retval 0=成功, 1=未检测到传感器
  */
uint8_t VL53L0X_Init(void);

/**
  * @brief  获取距离值 (连续测距模式下读取最新结果)
  * @retval 距离值, 单位: mm
  *         返回 0 表示数据无效或未就绪
  */
uint16_t VL53L0X_GetDistance(void);

#endif
