#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"
#include "LED.h"
#include "BH1750.h"
#include "VL53L0X.h"
#include "Buzzer.h"

/* 亮度映射参数 */
#define LUX_MAX         200     /* BH1750: >= 200 lux 时亮度为 0% */
#define DIST_MIN        100     /* VL53L0X: <= 100 mm 时最大亮度为 0% */
#define DIST_MAX        1200    /* VL53L0X: >= 1500 mm 时最大亮度为 100% */
#define PWM_PERIOD      999     /* TIM2 自动重装值 */
#define FADE_STEP       50      /* 渐变步长 (百分比/次, 约 0.4 秒走完全程) */
#define SMOOTH_NEW      5       /* 新读数权重 (5/10), 越大响应越快 */

int main(void)
{
    uint8_t vl53Status;
    uint16_t lux, dist;
    int16_t bright_pct;        /* BH1750 控制的百分比 0~100 */
    int16_t max_pct;           /* VL53L0X 限制的最大亮度 0~100 */
    uint16_t duty;             /* 最终 PWM 占空比 */
    uint16_t combined;         /* 合并后亮度 */
    uint16_t blink_cnt = 0;    /* PC13 闪烁计数 */
    uint16_t beep_cnt = 0;     /* 蜂鸣器计数 */
    static int16_t cur_comb = 0;   /* 当前实际亮度 (渐变过渡用) */
    static uint16_t sm_lux = 0;    /* 平滑后的照度 */
    static uint16_t sm_dist = 0;   /* 平滑后的距离 */

    OLED_Init();
    BH1750_Init();
    vl53Status = VL53L0X_Init();

    /* 初始化 PA1 PWM (TIM2_CH2) */
    LED1_Breath_Init();

    /* 初始化 PC13 板载状态灯 */
    LED_Status_Init();
    Buzzer_Init();
    if (vl53Status != 0) LED_Status_ON();  /* 异常: 常亮 */

    /* ====== OLED 固定布局 ====== */
    OLED_ShowString(1, 1, "BH1750:       lx");   /* 第9~13列: 5位数字, 第14列空格, 第15~16: lx */
    if (vl53Status == 0)
        OLED_ShowString(2, 1, "VL53L0X:      mm"); /* 第10~13列: 4位数字, 第14列空格, 第15~16: mm */
    else
        OLED_ShowString(2, 1, "VL53L0X: FAIL ");
    OLED_ShowString(3, 1, "LED:       %");       /* 第9~11列: 3位数字 */
    OLED_ShowString(4, 1, "PWM:     /999");       /* 第5~7列: 3位数字 */

    while (1)
    {
        /* ========== PC13 状态灯: dist<500mm 时快速闪烁 ========== */
        blink_cnt++;
        if (vl53Status == 0)
        {
            uint16_t interval = (dist < 500 && dist > 0) ? 1 : 5;
            if (blink_cnt >= interval) { LED_Status_Toggle(); blink_cnt = 0; }
        }

        /* ========== 读取传感器 ========== */
        lux = BH1750_GetLightIntensity();
        OLED_ShowNum(1, 9, lux, 5);           /* BH1750: xxxxx lx (原始值) */

        /* 低通滤波: sm = sm×(10-n)/10 + raw×n/10 */
        sm_lux = (uint16_t)(((uint32_t)sm_lux * (10 - SMOOTH_NEW) + (uint32_t)lux * SMOOTH_NEW) / 10);

        dist = 0;
        if (vl53Status == 0)
        {
            dist = VL53L0X_GetDistance();
            if (dist == 0xFFFF) dist = 0;
            OLED_ShowNum(2, 10, dist, 4);      /* VL53L0X: xxxx mm (原始值) */

            sm_dist = (uint16_t)(((uint32_t)sm_dist * (10 - SMOOTH_NEW) + (uint32_t)dist * SMOOTH_NEW) / 10);
        }
        else
        {
            sm_dist = 0;
        }

        /* ========== 蜂鸣器提示: dist<500mm 时周期性短鸣 ========== */
        if (vl53Status == 0 && dist < 500 && dist > 0)
        {
            beep_cnt++;
            if (beep_cnt >= 4)          /* 每 ~800ms 鸣响 ~200ms */
            {
                Buzzer_SetFreq(2000);
                Buzzer_SetVolume(500);
                beep_cnt = 0;
            }
            else
            {
                Buzzer_Off();
            }
        }
        else
        {
            Buzzer_Off();
            beep_cnt = 0;
        }

        /* ========== BH1750 → 亮度百分比 (使用平滑值) ========== */
        if (sm_lux >= LUX_MAX)
            bright_pct = 0;
        else
            bright_pct = (int16_t)((LUX_MAX - sm_lux) * 100 / LUX_MAX);

        /* ========== VL53L0X → 最大亮度限制 (使用平滑值) ========== */
        if (sm_dist >= DIST_MAX)
            max_pct = 100;
        else if (sm_dist <= DIST_MIN)
            max_pct = 0;
        else
            max_pct = (int16_t)((sm_dist - DIST_MIN) * 100 / (DIST_MAX - DIST_MIN));

        /* ========== 合并: 目标亮度 ========== */
        combined = (uint16_t)((uint32_t)bright_pct * max_pct / 100);

        /* ========== 渐变: 当前亮度逐步逼近目标亮度 ========== */
        if ((int16_t)combined > cur_comb)
        {
            cur_comb += FADE_STEP;
            if (cur_comb > (int16_t)combined) cur_comb = (int16_t)combined;
        }
        else if ((int16_t)combined < cur_comb)
        {
            cur_comb -= FADE_STEP;
            if (cur_comb < (int16_t)combined) cur_comb = (int16_t)combined;
        }

        duty = (uint16_t)((uint32_t)cur_comb * PWM_PERIOD / 100);
        TIM_SetCompare2(TIM2, duty);

        /* ========== OLED 数值更新 ========== */
        /* LED: xx% (右对齐, 避免前导零) */
        if (cur_comb >= 100)
            { OLED_ShowNum(3, 9, cur_comb, 3); }
        else if (cur_comb >= 10)
            { OLED_ShowString(3, 9, " "); OLED_ShowNum(3, 10, cur_comb, 2); }
        else
            { OLED_ShowString(3, 9, "  "); OLED_ShowNum(3, 11, cur_comb, 1); }

        /* PWM: xxx/999 */
        OLED_ShowNum(4, 5, duty, 3);

        Delay_ms(200);
    }
}
