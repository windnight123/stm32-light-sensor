#include "BH1750.h"
#include "stm32f10x.h"
#include "Delay.h"

/* I2C 引脚操作宏 (与 OLED 共用 PB6/PB7) */
#define BH1750_SCL_PIN    GPIO_Pin_6
#define BH1750_SDA_PIN    GPIO_Pin_7
#define BH1750_GPIO_PORT  GPIOB

#define BH1750_W_SCL(x)   GPIO_WriteBit(BH1750_GPIO_PORT, BH1750_SCL_PIN, (BitAction)(x))
#define BH1750_W_SDA(x)   GPIO_WriteBit(BH1750_GPIO_PORT, BH1750_SDA_PIN, (BitAction)(x))
#define BH1750_R_SDA()    GPIO_ReadInputDataBit(BH1750_GPIO_PORT, BH1750_SDA_PIN)

/* BH1750 8 位地址 (7 位地址左移 1 位) */
#define BH1750_ADDR_W     ((uint8_t)(BH1750_ADDR_DEFAULT << 1))       // 0x46
#define BH1750_ADDR_R     ((uint8_t)(BH1750_ADDR_DEFAULT << 1 | 1))   // 0x47

/* BH1750 指令 */
#define BH1750_CMD_POWER_ON       0x01
#define BH1750_CMD_RESET          0x07
#define BH1750_CMD_CONT_HRES      0x10    // 连续高分辨率模式, 1lux, 120ms

/* 延时等待测量完成 (H-Resolution 模式: 最大 180ms) */
#define BH1750_MEAS_DELAY_MS      180

/**
  * @brief  软件 I2C 起始信号
  */
static void BH1750_I2C_Start(void)
{
    BH1750_W_SDA(1);
    BH1750_W_SCL(1);
    Delay_us(5);
    BH1750_W_SDA(0);
    Delay_us(5);
    BH1750_W_SCL(0);
}

/**
  * @brief  软件 I2C 停止信号
  */
static void BH1750_I2C_Stop(void)
{
    BH1750_W_SDA(0);
    BH1750_W_SCL(1);
    Delay_us(5);
    BH1750_W_SDA(1);
    Delay_us(5);
}

/**
  * @brief  软件 I2C 发送一个字节 (不检查 ACK)
  * @param  Byte 要发送的字节
  */
static void BH1750_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        BH1750_W_SDA(Byte & (0x80 >> i));
        Delay_us(3);
        BH1750_W_SCL(1);
        Delay_us(3);
        BH1750_W_SCL(0);
    }
    /* 释放 SDA, 产生额外时钟供从机发送 ACK, 不处理应答 */
    BH1750_W_SDA(1);
    Delay_us(3);
    BH1750_W_SCL(1);
    Delay_us(3);
    BH1750_W_SCL(0);
}

/**
  * @brief  软件 I2C 读取一个字节
  * @retval 读取到的字节
  */
static uint8_t BH1750_I2C_ReadByte(void)
{
    uint8_t i, data = 0;
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        BH1750_W_SCL(1);
        Delay_us(3);
        data |= BH1750_R_SDA();
        BH1750_W_SCL(0);
        Delay_us(3);
    }
    return data;
}

/**
  * @brief  发送 ACK
  */
static void BH1750_I2C_Ack(void)
{
    BH1750_W_SDA(0);
    Delay_us(3);
    BH1750_W_SCL(1);
    Delay_us(3);
    BH1750_W_SCL(0);
    BH1750_W_SDA(1);
}

/**
  * @brief  发送 NACK
  */
static void BH1750_I2C_Nack(void)
{
    BH1750_W_SDA(1);
    Delay_us(3);
    BH1750_W_SCL(1);
    Delay_us(3);
    BH1750_W_SCL(0);
}

/**
  * @brief  初始化 BH1750 的 I2C GPIO 引脚
  * @note   OLED_Init() 已配置 PB6/PB7 为开漏输出,
  *         此处仅确保引脚状态正确, 避免重复配置。
  *         若要单独使用 BH1750 而不依赖 OLED,
  *         取消下面注释即可独立初始化。
  */
static void BH1750_GPIO_Init(void)
{
    /* OLED_Init() 已经配置了 PB6/PB7 为开漏输出 */
    /* 若 OLED 未初始化, 取消注释以下代码 */

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = BH1750_SCL_PIN;
    GPIO_Init(BH1750_GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = BH1750_SDA_PIN;
    GPIO_Init(BH1750_GPIO_PORT, &GPIO_InitStructure);

    BH1750_W_SCL(1);
    BH1750_W_SDA(1);
}

/**
  * @brief  向 BH1750 写入命令
  * @param  Cmd 命令字节
  */
static void BH1750_WriteCmd(uint8_t Cmd)
{
    BH1750_I2C_Start();
    BH1750_I2C_SendByte(BH1750_ADDR_W);
    BH1750_I2C_SendByte(Cmd);
    BH1750_I2C_Stop();
}

void BH1750_Init(void)
{
    BH1750_GPIO_Init();

    /* 上电 */
    BH1750_WriteCmd(BH1750_CMD_POWER_ON);
    Delay_ms(10);

    /* 复位 (清除之前可能残留的测量状态) */
    BH1750_WriteCmd(BH1750_CMD_RESET);
    Delay_ms(10);

    /* 设置为连续高分辨率模式 (1 lux, 测量时间 120ms) */
    BH1750_WriteCmd(BH1750_CMD_CONT_HRES);
    Delay_ms(BH1750_MEAS_DELAY_MS);
}

uint16_t BH1750_GetLightIntensity(void)
{
    uint16_t raw;
    uint8_t msb, lsb;

    /* 在连续模式下, 直接发起读操作 */
    BH1750_I2C_Start();
    BH1750_I2C_SendByte(BH1750_ADDR_R);

    /* 读取 2 个字节: MSB + LSB */
    msb = BH1750_I2C_ReadByte();
    BH1750_I2C_Ack();
    lsb = BH1750_I2C_ReadByte();
    BH1750_I2C_Nack();

    BH1750_I2C_Stop();

    raw = ((uint16_t)msb << 8) | lsb;

    /*
     * BH1750 高分辨率模式原始值转换公式:
     *   照度 (lux) = raw / 1.2
     * 这里使用整数运算: raw * 5 / 6
     */
    return (uint16_t)(((uint32_t)raw * 5) / 6);
}
