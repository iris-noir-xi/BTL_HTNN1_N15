#include "task_display.h"
#include "app_rtos.h"
#include "menu.h"

static void vTaskDisplay(void *pvParameters)
{
    uint8_t mode = MODE_CLOCK;
    int selection = 0;
    (void)pvParameters;

    for (;;)
    {
        if (xQueueReceive(xQueueAppMode, &mode, portMAX_DELAY) == pdTRUE)
        {
            if (mode == MODE_SETTIME)
            {
                SettingPage();
            }
            else if (mode == MODE_MENU)
            {
                while (1)
                {
                    selection = Menu();
                    if (selection == 0)
                    {
                        break;
                    }
                    else if (selection == 2)
                    {
                        MenuToFunction();
                        StopWatch();
                    }
                    else if (selection == 3)
                    {
                        MenuToFunction();
                        LED();
                    }
                    else if (selection == 4)
                    {
                        MenuToFunction();
                        MPU6050();
                    }
                    else if (selection == 5)
                    {
                        MenuToFunction();
                        Game();
                    }
                    else if (selection == 6)
                    {
                        MenuToFunction();
                        Emoji();
                    }
                    else if (selection == 7)
                    {
                        MenuToFunction();
                        Gradienter();
                    }
                    else
                    {
                        break;
                    }
                }
            }

            xSemaphoreGive(xSemAlarm);
        }
    }
}

void Task_Display_Init(void)
{
    xTaskCreate(vTaskDisplay, "Task_Display", 512, NULL, 2, NULL);
}