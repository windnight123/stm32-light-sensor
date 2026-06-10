#ifndef __BH1750_H
#define __BH1750_H

#include <stdint.h>

/*==========================================
 * BH1750 光强度传感器 (GY-302 模块)
 *
 * 引脚连接 (与 OLED 共用 I2C 总线):
 *   BH1750 SCL  ->  PB6  (与 OLED SCL 并联)
 *   BH1750 SDA  ->  PB7  (与 OLED SDA 并联)
 *   BH1750 VCC  ->  3.3V
 *   BH1750 GND  ->  GND
 *   BH1750 ADDR ->  GND (地址 0x23)
 *
 * OLED 引脚:
 *   OLED SCL   ->  PB6
 *   OLED SDA   ->  PB7
 *   OLED VCC   ->  3.3V
 *   OLED GND   ->  GND
 *==========================================*/

/* BH1750 地址 (ADDR=GND: 0x23, ADDR=VCC: 0x5C) */
#define BH1750_ADDR_DEFAULT  0x23

/**
  * @brief  初始化 BH1750 (配置 GPIO, 发送上电命令)
  */
void BH1750_Init(void);

/**
  * @brief  获取光强度值 (连续高分辨率模式)
  * @retval 亮度值, 单位: lux (范围 1~65535)
  *         返回 0 表示读取失败
  */
uint16_t BH1750_GetLightIntensity(void);

#endif
