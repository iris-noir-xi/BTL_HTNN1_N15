#include "gpio.h"
#include <stdint.h>

/* Bi?n toàn c?c d? freertos_demo.c liên k?t (Fix l?i L6218E) */
uint8_t Key_Num = 0;

uint8_t Key_GetNum(void)
{
    // Ki?m tra Phím 1 (PB1)
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
        for (volatile uint32_t i = 0; i < 20000; i++);
        if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET);
            Key_Num = 1;
            return 1;
        }
    }
    
    // Ki?m tra Phím 2 (PA6)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET)
    {
        for (volatile uint32_t i = 0; i < 20000; i++);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET);
            Key_Num = 2;
            return 2;
        }
    }
    
    // Ki?m tra Phím 3 (PA4)
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
    {
        uint32_t hold_cnt = 0;
        for (volatile uint32_t i = 0; i < 20000; i++);
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
        {
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
            {
                hold_cnt++;
                for (volatile uint32_t i = 0; i < 1000; i++);
                if (hold_cnt > 3000) break;
            }
            if (hold_cnt > 3000)
            {
                while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET);
                Key_Num = 4;
                return 4; // Nh?n gi?
            }
            Key_Num = 3;
            return 3; // Nh?n nh?
        }
    }
    
    return 0;
}

uint8_t Key_GetState(void)
{
    if (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) return 1;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET) return 2;
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET) return 3;
    return 0;
}

void Key_Tick(void) {}
void Key3_Tick(void) {}
	