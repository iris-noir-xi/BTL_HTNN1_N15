#include "task_time.h"
#include "app_rtos.h"
#include "menu.h"

static void vTaskTime(void *pvParameters)
{
    int flag = 0;
    uint8_t next_mode = MODE_CLOCK;
    (void)pvParameters;

    for (;;)
    {
        flag = First_Page_Clock();

        if (flag == 1)
        {
            next_mode = MODE_MENU;
            xQueueSend(xQueueAppMode, &next_mode, portMAX_DELAY);
            xSemaphoreTake(xSemAlarm, portMAX_DELAY);
        }
        else if (flag == 2)
        {
            next_mode = MODE_SETTIME;
            xQueueSend(xQueueAppMode, &next_mode, portMAX_DELAY);
            xSemaphoreTake(xSemAlarm, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void Task_Time_Init(void)
{
    xTaskCreate(vTaskTime, "Task_Time", 384, NULL, 2, NULL);
}