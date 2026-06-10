#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f10x.h"

/* 音符频率定义 (Hz) */
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_D6  1175
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_G6  1568
#define NOTE_A6  1760
#define NOTE_B6  1976
#define NOTE_C7  2093

/* 休止符 */
#define NOTE_NONE 0

void Buzzer_Init(void);
void Buzzer_SetFreq(uint16_t freq);
void Buzzer_SetVolume(uint16_t volume);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_PlayTone(uint16_t freq, uint16_t duration_ms);

#endif
