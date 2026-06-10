# STM32 光照+距离感应台灯

基于 STM32F103C8T6 (Blue Pill) 的智能台灯，根据环境光照自动调节 LED 亮度，同时通过激光测距实现距离保护。

## 硬件清单

| 组件 | 型号 | 接口 |
|------|------|------|
| 主控 | STM32F103C8T6 | - |
| 光照传感器 | BH1750 | I2C |
| 激光测距 | VL53L0X | I2C |
| OLED 显示屏 | 0.96" SSD1306 | I2C |
| LED 灯 | 高亮 LED (PA1) | TIM2_CH2 PWM |
| 无源蜂鸣器 | 低电平触发 (PA6) | TIM3_CH1 PWM |
| 板载指示灯 | PC13 | GPIO |

## 引脚分配

| GPIO | 功能 | 外设 |
|------|------|------|
| PA1 | LED PWM 调光 | TIM2_CH2 |
| PA6 | 蜂鸣器 PWM | TIM3_CH1 |
| PB6 | I2C1 时钟线 | 传感器 + OLED |
| PB7 | I2C1 数据线 | 传感器 + OLED |
| PC13 | 板载状态灯 | GPIO |

## 功能说明

- **环境光自适应**: BH1750 检测环境照度，越暗 LED 越亮（阈值 200 lux）
- **距离保护**: VL53L0X 检测障碍物距离，越近限制最大亮度（100~1500mm）
- **平滑渐变**: LED 亮度以渐变方式过渡，无突兀跳变
- **近距离报警**: 距离 < 500mm 时 PC13 快闪 + 蜂鸣器周期性短鸣
- **OLED 显示**: 实时显示照度、距离、亮度百分比和 PWM 占空比

## 开发环境

- IDE: Keil MDK (uvprojx)
- 标准外设库: STM32F10x_StdPeriph_Driver
- 晶振: 8MHz HSE → PLL x9 → 72MHz

## 目录结构

```
├── User/          主程序 (main.c, 中断, 配置)
├── Hardware/      外设驱动 (BH1750, VL53L0X, OLED, LED, Buzzer, Key)
├── Library/       标准外设库
├── Start/         启动文件 + 内核头文件
├── System/        延时工具
├── RTE/           Keil RTX 环境
└── Objects/       编译输出 (已忽略)
```
