#ifndef __LED_H
#define __LED_H

void LED_Init(void);
void LED1_ON(void);
void LED1_OFF(void);
void LED1_Turn(void);
void LED2_ON(void);
void LED2_OFF(void);
void LED2_Turn(void);
void LED1_Breath_Init(void);
void LED1_Breath(void);

/* PC13 板载状态指示灯 (低电平点亮) */
void LED_Status_Init(void);
void LED_Status_ON(void);
void LED_Status_OFF(void);
void LED_Status_Toggle(void);

#endif
