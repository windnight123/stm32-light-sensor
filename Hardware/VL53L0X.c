#include "VL53L0X.h"
#include "stm32f10x.h"
#include "Delay.h"

/* ============================================================
 * 基于 Polulu VL53L0X 库移植的纯软件 I2C 驱动
 * 共用 PB6(SCL) / PB7(SDA)
 * ============================================================ */

/* ======================== 引脚宏 ========================== */
#define SCL_PIN    GPIO_Pin_6
#define SDA_PIN    GPIO_Pin_7
#define GPIO_PORT  GPIOB

#define W_SCL(x)   GPIO_WriteBit(GPIO_PORT, SCL_PIN, (BitAction)(x))
#define W_SDA(x)   GPIO_WriteBit(GPIO_PORT, SDA_PIN, (BitAction)(x))
#define R_SDA()    GPIO_ReadInputDataBit(GPIO_PORT, SDA_PIN)

/* ====================== I2C 地址 ========================= */
#define ADDR_W     0x52
#define ADDR_R     0x53

/* ======================= 寄存器 =========================== */
#define REG_IDENTIFICATION_MODEL_ID         0xC0
#define REG_SYSRANGE_START                  0x00
#define REG_RESULT_INTERRUPT_STATUS         0x13
#define REG_RESULT_RANGE_STATUS             0x14
#define REG_SYSTEM_INTERRUPT_CLEAR          0x0B

#define REG_SOFT_RESET_GO2_SOFT_RESET_N     0xBF
#define REG_IDENTIFICATION_REVISION_ID      0xC2
#define REG_VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV  0x89

#define REG_MSRC_CONFIG_CONTROL             0x60
#define REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define REG_SYSTEM_SEQUENCE_CONFIG          0x01
#define REG_DYNAMIC_SPAD_REF_EN_START_OFFSET   0x4F
#define REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD  0x4E
#define REG_GLOBAL_CONFIG_REF_EN_START_SELECT   0xB6
#define REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0    0xB0
#define REG_SYSTEM_INTERRUPT_CONFIG_GPIO        0x0A
#define REG_GPIO_HV_MUX_ACTIVE_HIGH            0x84

/* ====================== 软件 I2C ========================= */
static void I2C_Start(void)
{
    W_SDA(1); W_SCL(1); Delay_us(4);
    W_SDA(0);             Delay_us(4);
    W_SCL(0);
}

static void I2C_Stop(void)
{
    W_SDA(0); W_SCL(1);  Delay_us(4);
    W_SDA(1);             Delay_us(4);
}

static void I2C_SendByte(uint8_t byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        W_SDA(byte & (0x80 >> i));
        Delay_us(2);
        W_SCL(1); Delay_us(2);
        W_SCL(0); Delay_us(2);
    }
    W_SDA(1);
    Delay_us(2);
    W_SCL(1); Delay_us(2);
    W_SCL(0); Delay_us(2);
}

static uint8_t I2C_ReadByte(void)
{
    uint8_t i, data = 0;
    for (i = 0; i < 8; i++)
    {
        data <<= 1;
        W_SCL(1); Delay_us(2);
        data |= R_SDA();
        W_SCL(0); Delay_us(2);
    }
    return data;
}

static void I2C_Ack(void)
{
    W_SDA(0); Delay_us(2);
    W_SCL(1); Delay_us(2);
    W_SCL(0); Delay_us(2);
    W_SDA(1);
}

static void I2C_Nack(void)
{
    W_SDA(1); Delay_us(2);
    W_SCL(1); Delay_us(2);
    W_SCL(0); Delay_us(2);
}

/* ==================== 寄存器读写 ========================== */
static void WriteReg(uint8_t reg, uint8_t val)
{
    I2C_Start();
    I2C_SendByte(ADDR_W);
    I2C_SendByte(reg);
    I2C_SendByte(val);
    I2C_Stop();
}

static uint8_t ReadReg(uint8_t reg)
{
    uint8_t val;
    I2C_Start();
    I2C_SendByte(ADDR_W);
    I2C_SendByte(reg);
    I2C_Start();
    I2C_SendByte(ADDR_R);
    val = I2C_ReadByte();
    I2C_Nack();
    I2C_Stop();
    return val;
}

static uint16_t ReadReg16(uint8_t reg)
{
    uint16_t val;
    I2C_Start();
    I2C_SendByte(ADDR_W);
    I2C_SendByte(reg);
    I2C_Start();
    I2C_SendByte(ADDR_R);
    val = (uint16_t)I2C_ReadByte() << 8;
    I2C_Ack();
    val |= I2C_ReadByte();
    I2C_Nack();
    I2C_Stop();
    return val;
}

static void ReadMulti(uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t i;
    I2C_Start();
    I2C_SendByte(ADDR_W);
    I2C_SendByte(reg);
    I2C_Start();
    I2C_SendByte(ADDR_R);
    for (i = 0; i < len; i++)
    {
        buf[i] = I2C_ReadByte();
        if (i < len - 1) I2C_Ack(); else I2C_Nack();
    }
    I2C_Stop();
}

static void WriteMulti(uint8_t reg, const uint8_t *buf, uint8_t len)
{
    uint8_t i;
    I2C_Start();
    I2C_SendByte(ADDR_W);
    I2C_SendByte(reg);
    for (i = 0; i < len; i++) I2C_SendByte(buf[i]);
    I2C_Stop();
}

/* ==================== GPIO 初始化 ========================= */
static void GPIO_Init_(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = SCL_PIN;
    GPIO_Init(GPIO_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.GPIO_Pin = SDA_PIN;
    GPIO_Init(GPIO_PORT, &GPIO_InitStructure);
    W_SCL(1);
    W_SDA(1);
}

/* =================== 全局状态 ============================== */
static uint8_t stop_variable = 0;

/* ==================== 超时工具 ============================ */
static uint8_t WaitForBit(uint8_t reg, uint8_t mask, uint32_t timeout_ms)
{
    while (timeout_ms--)
    {
        if (ReadReg(reg) & mask) return 0;
        Delay_ms(1);
    }
    return 1;
}
 
/* ===================== SPAD 配置 =========================== */
/* 使用固定默认配置, 跳过 OTP 读取 (避免部分模块 OTP 时序不兼容) */
static void VL53L0X_SpadConfig(void)
{
    uint8_t spad_map[6];

    /* 读取当前 SPAD 启用映射 (直接用传感器出厂默认值) */
    ReadMulti(REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_map, 6);

    /* 保持原有的 SPAD 配置回写 */
    WriteReg(0xFF, 0x01);
    WriteReg(REG_DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    WriteReg(REG_DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    WriteReg(0xFF, 0x00);
    WriteReg(REG_GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

    WriteMulti(REG_GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spad_map, 6);
}

/* ============ 单次参考校准 (VHV / Phase) ==================== */
static void VL53L0X_PerformSingleRefCalibration(uint8_t cal_type)
{
    WriteReg(REG_SYSRANGE_START, 0x01 | cal_type);
    WaitForBit(REG_RESULT_INTERRUPT_STATUS, 0x07, 500);
    WriteReg(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    WriteReg(REG_SYSRANGE_START, 0x00);
}

/* ============================================================
 *  公开 API
 * ============================================================ */

uint8_t VL53L0X_Init(void)
{
    uint8_t id;
    uint8_t retry;

    GPIO_Init_();
    Delay_ms(100);

    /* 重试 3 次读取 ID */
    for (retry = 0; retry < 3; retry++)
    {
        id = ReadReg(REG_IDENTIFICATION_MODEL_ID);
        if (id == 0xEE) break;
        Delay_ms(50);
    }
    if (id != 0xEE) return 1;

    /* === 2. DataInit (来自 Pololu) === */
    WriteReg(0x88, 0x00);

    WriteReg(0x80, 0x01);
    WriteReg(0xFF, 0x01);
    WriteReg(0x00, 0x00);
    stop_variable = ReadReg(0x91);              /* 关键: 保存此值 */
    WriteReg(0x00, 0x01);
    WriteReg(0xFF, 0x00);
    WriteReg(0x80, 0x00);

    /* MSRC 控制: 设置 bit1 和 bit4 */
    WriteReg(REG_MSRC_CONFIG_CONTROL, ReadReg(REG_MSRC_CONFIG_CONTROL) | 0x12);

    /* 信号速率限制: 0.25 MCPS = 0x0020 */
    WriteReg(REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, 0x00);
    WriteReg(REG_FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT + 1, 0x20);

    WriteReg(REG_SYSTEM_SEQUENCE_CONFIG, 0xFF);
    Delay_ms(10);

    /* === 3. SPAD 配置 (跳过 OTP 读取, 用出厂默认值) === */
    VL53L0X_SpadConfig();

    /* === 4. 静态调优配置 (完整序列) === */
    WriteReg(0xFF, 0x01);
    WriteReg(0x00, 0x00);
    WriteReg(0xFF, 0x00);
    WriteReg(0x09, 0x00);
    WriteReg(0x10, 0x00);
    WriteReg(0x11, 0x00);

    WriteReg(0x24, 0x01);
    WriteReg(0x25, 0xFF);
    WriteReg(0x75, 0x00);

    WriteReg(0xFF, 0x01);
    WriteReg(0x4E, 0x2C);
    WriteReg(0x48, 0x00);
    WriteReg(0x30, 0x20);

    WriteReg(0xFF, 0x00);
    WriteReg(0x30, 0x09);
    WriteReg(0x54, 0x00);
    WriteReg(0x31, 0x04);
    WriteReg(0x32, 0x03);
    WriteReg(0x40, 0x83);
    WriteReg(0x46, 0x25);
    WriteReg(0x60, 0x00);
    WriteReg(0x27, 0x00);
    WriteReg(0x50, 0x06);
    WriteReg(0x51, 0x00);
    WriteReg(0x52, 0x96);
    WriteReg(0x56, 0x08);
    WriteReg(0x57, 0x30);
    WriteReg(0x61, 0x00);
    WriteReg(0x62, 0x00);
    WriteReg(0x64, 0x00);
    WriteReg(0x65, 0x00);
    WriteReg(0x66, 0xA0);

    WriteReg(0xFF, 0x01);
    WriteReg(0x22, 0x32);
    WriteReg(0x47, 0x14);
    WriteReg(0x49, 0xFF);
    WriteReg(0x4A, 0x00);

    WriteReg(0xFF, 0x00);
    WriteReg(0x7A, 0x0A);
    WriteReg(0x7B, 0x00);
    WriteReg(0x78, 0x21);

    WriteReg(0xFF, 0x01);
    WriteReg(0x23, 0x34);
    WriteReg(0x42, 0x00);
    WriteReg(0x44, 0xFF);
    WriteReg(0x45, 0x26);
    WriteReg(0x46, 0x05);
    WriteReg(0x40, 0x40);
    WriteReg(0x0E, 0x06);
    WriteReg(0x20, 0x1A);
    WriteReg(0x43, 0x40);

    WriteReg(0xFF, 0x00);
    WriteReg(0x34, 0x03);
    WriteReg(0x35, 0x44);

    WriteReg(0xFF, 0x01);
    WriteReg(0x31, 0x04);
    WriteReg(0x4B, 0x09);
    WriteReg(0x4C, 0x05);
    WriteReg(0x4D, 0x04);

    WriteReg(0xFF, 0x00);
    WriteReg(0x44, 0x00);
    WriteReg(0x45, 0x20);
    WriteReg(0x47, 0x08);
    WriteReg(0x48, 0x28);
    WriteReg(0x67, 0x00);
    WriteReg(0x70, 0x04);
    WriteReg(0x71, 0x01);
    WriteReg(0x72, 0xFE);
    WriteReg(0x76, 0x00);
    WriteReg(0x77, 0x00);

    WriteReg(0xFF, 0x01);
    WriteReg(0x0D, 0x01);

    WriteReg(0xFF, 0x00);
    WriteReg(0x80, 0x01);
    WriteReg(0x01, 0xF8);

    WriteReg(0xFF, 0x01);
    WriteReg(0x8E, 0x01);
    WriteReg(0x00, 0x01);

    WriteReg(0xFF, 0x00);
    WriteReg(0x80, 0x00);

    /* === 5. GPIO 中断配置 === */
    WriteReg(REG_GPIO_HV_MUX_ACTIVE_HIGH, ReadReg(REG_GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    WriteReg(REG_SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);   /* 新采样就绪时触发 */
    WriteReg(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);
    WriteReg(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);          /* TCC+MSRC+DSS+Pre+Final */

    /* === 6. VHV 校准 === */
    WriteReg(REG_SYSTEM_SEQUENCE_CONFIG, 0x01);
    VL53L0X_PerformSingleRefCalibration(0x40);
    Delay_ms(10);

    /* === 7. Phase 校准 === */
    WriteReg(REG_SYSTEM_SEQUENCE_CONFIG, 0x02);
    VL53L0X_PerformSingleRefCalibration(0x00);
    Delay_ms(10);

    /* === 8. 恢复完整序列 === */
    WriteReg(REG_SYSTEM_SEQUENCE_CONFIG, 0xE8);

    return 0;
}

uint16_t VL53L0X_GetDistance(void)
{
    uint16_t dist;

    /* === 单次测距: 停止→恢复 stop_variable→启动 === */
    WriteReg(0x80, 0x01);
    WriteReg(0xFF, 0x01);
    WriteReg(0x00, 0x00);
    WriteReg(0x91, stop_variable);            /* 恢复保存的变量 */
    WriteReg(0x00, 0x01);
    WriteReg(0xFF, 0x00);
    WriteReg(0x80, 0x00);
    WriteReg(REG_SYSRANGE_START, 0x01);        /* 触发单次测距 */

    /* 等待 SYSRANGE_START bit0 清除 (测量完成) */
    {
        uint16_t timeout = 200;
        while (timeout--)
        {
            if (!(ReadReg(REG_SYSRANGE_START) & 0x01)) break;
            Delay_ms(1);
        }
        if (timeout == 0) return 0xFFFF;        /* 测距超时 */
    }

    /* 等待中断状态非零 */
    if (WaitForBit(REG_RESULT_INTERRUPT_STATUS, 0x07, 100)) return 0xFFFF;

    /* 读取距离: RESULT_RANGE_STATUS + 10 = 0x1E */
    dist = ReadReg16(REG_RESULT_RANGE_STATUS + 10);

    /* 清除中断 */
    WriteReg(REG_SYSTEM_INTERRUPT_CLEAR, 0x01);

    return dist;
}
