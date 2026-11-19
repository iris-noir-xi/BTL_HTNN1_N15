#include "password.h"
#include "adc.h"
#include "OLED.h"
#include "gpio.h"
#include <math.h>
#include <stdlib.h>

#define MAX_KNOCKS          8       // S? l?n gõ t?i da cho phép
#define KNOCK_TIMEOUT_MS    1200    // Quá 1.2s không gõ n?a thì ti?n hành ki?m tra

volatile uint8_t is_device_locked = 1; // M?c d?nh b?t máy là khóa màn hình

/* M?t mã m?u: 4 l?n gõ t?o thành 3 kho?ng ngh? có t? l?: [1.0, 2.0, 1.0] (Ng?n - Dài - Ng?n) */
static const float secret_pattern[3] = {1.0f, 2.0f, 1.0f};
#define PATTERN_LEN 3

static uint32_t intervals[MAX_KNOCKS];
static uint8_t knock_count = 0;
static TickType_t last_knock_time = 0;
static uint8_t is_covered = 0;

/* Ð?c ADC t? c?m bi?n quang / h?ng ngo?i (PA0) */
static uint16_t Read_Optical_Sensor(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 2);
    return HAL_ADC_GetValue(&hadc1);
}

/* Hàm so kh?p m?t kh?u ti?t t?u */
static uint8_t Verify_Password(void)
{
    // N?u s? kho?ng cách không kh?p v?i d? dài m?t kh?u m?u
    if ((knock_count - 1) != PATTERN_LEN)
    {
        return 0; // Sai s? lu?ng nh?p
    }

    // L?y kho?ng th?i gian c?a nh?p d?u tiên làm chu?n
    float base_interval = (float)intervals[0];
    if (base_interval < 50.0f) return 0; // Nh?p quá ng?n do nhi?u

    // So sánh t? l? t?ng kho?ng ngh?
    for (uint8_t i = 0; i < PATTERN_LEN; i++)
    {
        float ratio = (float)intervals[i] / base_interval;
        // Cho phép dung sai chênh l?ch ±0.35 (35%)
        if (fabs(ratio - secret_pattern[i]) > 0.35f)
        {
            return 0; // Sai ti?t t?u
        }
    }

    return 1; // Kh?p m?t mã hoàn toàn
}

/* Giao di?n hi?n th? màn hình khóa */
void Show_Lock_Screen_UI(uint8_t count)
{
    OLED_Clear();
    OLED_ShowString(2, 4, "[ BI KHOA ]", 8);
    OLED_ShowString(3, 1, "GO NHIP DE MO", 8);

    // V? các d?u sao/ch?m tuong ?ng s? nh?p gõ nh?n du?c
    if (count > 0)
    {
        char dot_str[16];
        uint8_t i;
        for (i = 0; i < count && i < 12; i++)
        {
            dot_str[i] = '*';
        }
        dot_str[i] = '\0';
        OLED_ShowString(4, 8 - (count / 2), dot_str, 8);
    }
    else
    {
        OLED_ShowString(4, 3, "DANG CHO...", 8);
    }

    OLED_Update();
}

void Password_Init(void)
{
    is_device_locked = 1;
    knock_count = 0;
    is_covered = 0;
}

/* Task FreeRTOS ch?y n?n giám sát c?m bi?n quang */
void Password_Task(void *pvParameters)
{
    Password_Init();

    for (;;)
    {
        // Khi thi?t b? dang b? khóa, ti?n hành quét c?m bi?n
        if (is_device_locked)
        {
            uint16_t adc_val = Read_Optical_Sensor();
            TickType_t now = xTaskGetTickCount();

            // Ngu?ng phát hi?n che sáng (tùy môi tru?ng sáng t?i, ví d? > 1200 là b? che)
            // Dùng co ch? Schmitt trigger ch?ng rung tay:
            if (adc_val > 1500 && !is_covered)
            {
                is_covered = 1;
                
                if (knock_count == 0)
                {
                    last_knock_time = now;
                    knock_count = 1;
                }
                else if (knock_count < MAX_KNOCKS)
                {
                    // Tính kho?ng cách th?i gian gi?a 2 l?n gõ (ms)
                    intervals[knock_count - 1] = (now - last_knock_time) * portTICK_PERIOD_MS;
                    last_knock_time = now;
                    knock_count++;
                }

                // Nh?p nháy LED ho?c còi báo dã nh?n 1 nh?p gõ
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
                vTaskDelay(pdMS_TO_TICKS(30));
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);

                Show_Lock_Screen_UI(knock_count);
            }
            else if (adc_val < 800 && is_covered)
            {
                is_covered = 0; // Ðã nh?c ngón tay ra
            }

            // Ki?m tra Timeout: N?u dã gõ và d?ng l?i quá 1.2 giây
            if (knock_count > 1 && ((now - last_knock_time) * portTICK_PERIOD_MS > KNOCK_TIMEOUT_MS))
            {
                if (Verify_Password() == 1)
                {
                    // M? KHÓA THÀNH CÔNG
                    OLED_Clear();
                    OLED_ShowString(2, 3, "MO KHOA OK!", 8);
                    OLED_Update();

                    is_device_locked = 0; // M? khóa cho phép vào giao di?n chính
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                else
                {
                    // SAI M?T KH?U
                    OLED_Clear();
                    OLED_ShowString(2, 3, "SAI MAT MA!", 8);
                    OLED_Update();
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    Show_Lock_Screen_UI(0);
                }

                // Reset bi?n d?m nh?p
                knock_count = 0;
            }

            vTaskDelay(pdMS_TO_TICKS(15)); // Quét chu k? 15ms r?t nh? CPU
        }
        else
        {
            // Khi dã m? khóa, task ch? ng? d?nh k? d? ti?t ki?m tài nguyên
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}