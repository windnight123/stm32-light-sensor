#include "stm32f10x.h"
#include "Delay.h"
#include "OLED.h"

void LED_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	GPIO_SetBits(GPIOA, GPIO_Pin_1 | GPIO_Pin_2);
}

void LED1_ON(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_1);
}

void LED1_OFF(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_1);
}

void LED1_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1) == 0)
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_1);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_1);
	}
}

void LED2_ON(void)
{
	GPIO_ResetBits(GPIOA, GPIO_Pin_2);
}

void LED2_OFF(void)
{
	GPIO_SetBits(GPIOA, GPIO_Pin_2);
}

void LED2_Turn(void)
{
	if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_2) == 0)
	{
		GPIO_SetBits(GPIOA, GPIO_Pin_2);
	}
	else
	{
		GPIO_ResetBits(GPIOA, GPIO_Pin_2);
	}
}

/**
  * @brief  LED1呼吸灯初始化（PA1 -> TIM2_CH2 PWM）
  * @param  无
  * @retval 无
  */
void LED1_Breath_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	TIM_TimeBaseStructure.TIM_Prescaler = 71;		// 72MHz / 72 = 1MHz
	TIM_TimeBaseStructure.TIM_Period = 999;			// 1MHz / 1000 = 1kHz PWM
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_OCInitTypeDef TIM_OCInitStructure;
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);

	TIM_ARRPreloadConfig(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);
}

/**
  * @brief  LED1呼吸一次（从暗到亮后熄灭），同时显示占空比
  * @param  无
  * @retval 无
  */
void LED1_Breath(void)
{
	uint16_t duty;
	uint8_t percent;

	OLED_ShowString(4, 1, "Duty:    %");

	/* 从暗到亮，逐渐增加占空比 */
	for (duty = 0; duty <= 999; duty++)
	{
		TIM_SetCompare2(TIM2, duty);
		percent = (uint8_t)((duty + 1) * 100 / 1000);	// 0~100
		OLED_ShowNum(4, 7, percent, 3);
		Delay_ms(2);
	}

	/* 到达最高亮度后熄灭 */
	TIM_SetCompare2(TIM2, 0);
	OLED_ShowString(4, 1, "Duty:   0%");
}

/* ==================== PC13 板载状态灯 ==================== */

void LED_Status_Init(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_SetBits(GPIOC, GPIO_Pin_13);	/* 初始化: 灭 (高电平) */
}

void LED_Status_ON(void)
{
	GPIO_ResetBits(GPIOC, GPIO_Pin_13);	/* 低电平点亮 */
}

void LED_Status_OFF(void)
{
	GPIO_SetBits(GPIOC, GPIO_Pin_13);	/* 高电平熄灭 */
}

void LED_Status_Toggle(void)
{
	if (GPIO_ReadOutputDataBit(GPIOC, GPIO_Pin_13) == 0)
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
	else
		GPIO_ResetBits(GPIOC, GPIO_Pin_13);
}
