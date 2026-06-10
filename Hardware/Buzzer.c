#include "Buzzer.h"
#include "Delay.h"

/**
  * @brief  无源蜂鸣器初始化 (PA6 -> TIM3_CH1 PWM)
  *         定时器时钟 72MHz, 预分频 71 -> 1MHz
  *         通过改变 ARR 调节频率, CCR 调节占空比
  * @param  无
  * @retval 无
  */
void Buzzer_Init(void)
{
    /* 使能 GPIOA 和 TIM3 时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* PA6 复用推挽输出 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* TIM3 时基配置: 72MHz / (71+1) = 1MHz */
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_TimeBaseStructure.TIM_Prescaler = 71;          // 1MHz 计数时钟
    TIM_TimeBaseStructure.TIM_Period = 999;             // 默认 1kHz (占位)
    TIM_TimeBaseStructure.TIM_ClockDivision = 0;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* TIM3_CH1 PWM1 模式 */
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;                  // 默认关闭
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;  /* 低电平触发 */
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);

    TIM_ARRPreloadConfig(TIM3, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  设置蜂鸣器频率
  * @param  freq 频率 (Hz), 范围 ~100-10000
  * @retval 无
  */
void Buzzer_SetFreq(uint16_t freq)
{
    if (freq == 0)
    {
        Buzzer_Off();
        return;
    }

    /* 1MHz / freq = 周期计数值 */
    uint16_t period = 1000000 / freq;
    TIM_SetAutoreload(TIM3, period - 1);

    /* 保持 50% 占空比 */
    TIM_SetCompare1(TIM3, period / 2);
}

/**
  * @brief  设置音量 (占空比)
  * @param  volume 占空比 0~1000 (0=静音, 500=50%, 1000=100%)
  * @retval 无
  */
void Buzzer_SetVolume(uint16_t volume)
{
    if (volume > 1000) volume = 1000;

    uint16_t period = TIM3->ARR + 1;
    uint16_t pulse = (uint32_t)period * volume / 1000;

    TIM_SetCompare1(TIM3, pulse);
}

/**
  * @brief  开启蜂鸣器 (恢复输出)
  * @param  无
  * @retval 无
  */
void Buzzer_On(void)
{
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  关闭蜂鸣器 (静音)
  * @param  无
  * @retval 无
  */
void Buzzer_Off(void)
{
    TIM_SetCompare1(TIM3, 0);
}

/**
  * @brief  播放一个音符
  * @param  freq         频率 (Hz), NOTE_NONE=休止
  * @param  duration_ms  持续时间 (ms)
  * @retval 无
  */
void Buzzer_PlayTone(uint16_t freq, uint16_t duration_ms)
{
    if (freq == 0)
    {
        /* 休止符: 静音等待 */
        TIM_SetCompare1(TIM3, 0);
        Delay_ms(duration_ms);
    }
    else
    {
        Buzzer_SetFreq(freq);
        Delay_ms(duration_ms);
        /* 短暂间隔防止音符粘连 */
        TIM_SetCompare1(TIM3, 0);
        Delay_ms(5);
    }
}
