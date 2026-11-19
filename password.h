#ifndef __PASSWORD_H
#define __PASSWORD_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"

/* C? tr?ng thái khóa màn hình: 1 = Ðang khóa, 0 = Ðã m? */
extern volatile uint8_t is_device_locked;

/* Kh?i t?o và Task FreeRTOS */
void Password_Init(void);
void Password_Task(void *pvParameters);

/* Hàm v? giao di?n khóa trên màn hình OLED */
void Show_Lock_Screen_UI(uint8_t knock_count);

#endif