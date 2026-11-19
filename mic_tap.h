#ifndef __MIC_TAP_H
#define __MIC_TAP_H

#include <stdint.h>

void Screen_Power_Init(void);
void Screen_TurnOn(void);
void Screen_TurnOff(void);
uint8_t Screen_IsOn(void);
void Screen_Toggle(void);

void Alarm_Trigger(void);
void Alarm_Stop(void);
uint8_t Alarm_IsRinging(void);

void Hardware_Sensors_Update(void);

#endif