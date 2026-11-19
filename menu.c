#include "FreeRTOS.h"
#include "task.h"
#include "rtc.h"
#include "OLED.h"
#include "Key.h"
#include "usart.h"
#include "SetTime.h"
#include "menu.h"
#include "gpio.h"
#include "MPU6050.h"
#include <math.h>
#include <stdlib.h>
#include "dino.h"
#include "adc.h"
#include "mic_tap.h"

RTC_DateTypeDef RTC_Date;
RTC_TimeTypeDef RTC_Time;

uint8_t num = 0;

extern int Battery_Capacity;
extern uint8_t sys_alarm_hour;
extern uint8_t sys_alarm_min;
extern uint8_t sys_alarm_enable;
extern uint8_t MyRTC_Time[];
extern uint16_t current_light_adc;

extern void Alarm_Trigger(void);
extern void Alarm_Stop(void);
extern uint8_t Alarm_IsRinging(void);
extern void Alarm_Ringing_Screen(void);

static void RTC_Sync_From_MyRTC(void)
{
    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    sDate.Year    = MyRTC_Time[0];
    sDate.Month   = MyRTC_Time[1];
    sDate.Date    = MyRTC_Time[2];
    sTime.Hours   = MyRTC_Time[3];
    sTime.Minutes = MyRTC_Time[4];
    sTime.Seconds = MyRTC_Time[5];

    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
}

const uint8_t Return[] = {
    0x00,0x00,0x80,0xC0,0xE0,0xF0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0xF8,0x00,
    0x00,0x01,0x03,0x07,0x0F,0x1F,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,
};

/* Icon chuong bao thuc mini 8x8 hien thi tren thanh trang thai */
const uint8_t Mini_Bell_Icon[8] = {
    0x20, 0x38, 0x3C, 0x7F, 0x7F, 0x3C, 0x38, 0x20
};

const uint8_t Menu_Graph[][128] = {
    // 0: Tro ve
    0x00,0x00,0x00,0x00,0xE0,0xF0,0xF0,0xF0,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,
    0x70,0x70,0x70,0x70,0x70,0x70,0x70,0x70,0xF0,0xF0,0xF0,0xE0,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xC0,0xC0,0xC0,0xC0,0xC0,0xC0,0xC3,0xFB,0xF3,0xE0,0xC0,0x80,0x00,
    0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x03,0x03,0x03,0x03,0x03,0x03,0xC3,0xDF,0xCF,0x07,0x03,0x01,0x00,
    0x00,0x00,0x00,0x00,0x07,0x0F,0x0F,0x0F,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,
    0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0E,0x0F,0x0F,0x0F,0x07,0x00,0x00,0x00,0x00,
    // 1: Bam gio
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x80,0x8E,0xDE,0xD3,0xF1,0xF9,0xD9,
    0xD9,0xF9,0xF1,0xD3,0xDF,0x8E,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x80,0xE0,0xF0,0x38,0x1C,0x0E,0x07,0x03,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0xFB,
    0xFB,0x00,0x00,0x00,0x00,0x01,0x01,0x01,0x03,0x07,0x06,0x0C,0x3E,0xFE,0xEC,0x80,
    0x7F,0xFF,0xCC,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,
    0x0F,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00,0x00,0x0C,0xCC,0xFF,0x7F,
    0x00,0x01,0x03,0x07,0x0C,0x18,0x38,0x30,0x60,0x60,0xC0,0xC0,0xF0,0xF0,0xF0,0xF0,
    0xF0,0xF0,0xF0,0xF0,0xC0,0xC0,0x60,0x60,0x30,0x30,0x18,0x0C,0x0F,0x03,0x01,0x00,
    // 2: Den pin
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,
    0xC0,0x70,0x70,0xE0,0x40,0x9F,0x1F,0x00,0x40,0x60,0x70,0x38,0x10,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0x7C,0xFF,0xC7,
    0x86,0x0C,0x18,0x30,0x61,0xC3,0x87,0x0E,0x9C,0xF8,0xF0,0x66,0x06,0x06,0x06,0x06,
    0x00,0x00,0x00,0xB0,0xF8,0xFC,0x7E,0x3F,0x1F,0x0E,0x07,0x03,0x81,0xC0,0x60,0x31,
    0x1B,0x0F,0x06,0x06,0x06,0x07,0x03,0x03,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x1E,0x3B,0x73,0xE1,0xC0,0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // 3: MPU6050
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xC0,0xC0,0xC0,0x00,
    0x00,0x00,0x0C,0x0C,0x3C,0x64,0xC4,0x86,0x86,0xC6,0x42,0x62,0x62,0x23,0xB3,0xD3,
    0x59,0x69,0x29,0x3D,0x1D,0x1D,0x0E,0x07,0x87,0xC3,0x73,0x19,0x0E,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0xF8,0xCC,0x4E,0x72,0x63,0xC1,0x01,0x00,
    0x00,0x00,0x00,0x80,0xC0,0x70,0x1C,0x06,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x01,0x03,0x06,
    0x0C,0x18,0x06,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    // 4: Game Dino
    0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x40,0x20,0x20,0x60,0xC0,0x80,0x80,0x00,0x00,
    0x00,0x00,0x00,0x80,0x80,0xC0,0x60,0x20,0x20,0x40,0xC0,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xF0,0x16,0x03,0xC0,0xC0,0xF8,0xF8,0xF8,0xC0,0xC0,0x01,0x01,0x03,
    0x03,0x03,0x01,0x01,0x80,0xC0,0xF0,0x70,0xF0,0xC0,0x80,0x03,0x1E,0xF0,0x80,0x00,
    0x00,0x00,0xFF,0x80,0x00,0x00,0x01,0x01,0x0B,0x8F,0xCB,0x61,0x21,0x20,0x30,0x30,
    0x30,0x20,0x30,0x20,0x21,0x43,0xCF,0x8E,0x0F,0x03,0x01,0x00,0x00,0x03,0xFF,0x00,
    0x00,0x00,0x00,0x03,0x04,0x0C,0x0C,0x04,0x06,0x01,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x04,0x0C,0x0C,0x04,0x03,0x00,0x00,
    // 5: Dong ho bao thuc
    0x00,0x00,0x00,0x00,0x38,0x7C,0xFE,0xFE,0xEE,0xC6,0x86,0x8C,0x98,0xB0,0xA0,0xE0,
    0xE0,0xA0,0xB0,0x98,0x8C,0x86,0xC6,0xEE,0xFE,0xFE,0x7C,0x38,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xE0,0xF8,0x1E,0x07,0x03,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0xFF,
    0xFF,0x00,0x00,0x00,0x00,0x00,0x01,0x01,0x03,0x07,0x1E,0xF8,0xE0,0x00,0x00,0x00,
    0x00,0x00,0x00,0x07,0x1F,0x78,0xE0,0xC0,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x01,
    0x01,0x03,0x06,0x0C,0x18,0x00,0x80,0x80,0xC0,0xE0,0x78,0x1F,0x07,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x18,0x1C,0x0E,0x07,0x03,0x03,0x06,0x06,0x06,0x06,0x06,
    0x06,0x06,0x06,0x06,0x06,0x03,0x03,0x07,0x0E,0x1C,0x18,0x00,0x00,0x00,0x00,0x00,
    // 6: Bot nuoc can bang
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xC0,0xE0,0xE0,0x70,0x78,0x38,0x38,0xF8,
    0x38,0x38,0x78,0x78,0x70,0xE0,0xC0,0xC0,0xC0,0xC0,0x80,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0xF0,0xFC,0x3E,0x07,0x63,0x19,0x04,0x02,0x02,0x01,0x00,0x01,0x9F,
    0x00,0x00,0x01,0x00,0x02,0x04,0x09,0x31,0x87,0x0F,0xFF,0xFE,0xFC,0x18,0x00,0x00,
    0x00,0x00,0x00,0x3F,0xFF,0xF1,0xC1,0x99,0x21,0x41,0x81,0x01,0x00,0x00,0x00,0xF1,
    0x00,0x00,0x00,0x01,0x81,0xC1,0x21,0x19,0x83,0xE1,0xFF,0x7F,0x0F,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x01,0x03,0x07,0x0F,0x0E,0x1C,0x1D,0x19,0x38,0x38,0x3F,
    0x38,0x38,0x19,0x1D,0x1C,0x0C,0x0E,0x07,0x03,0x03,0x00,0x00,0x00,0x00,0x00,0x00
};

const uint8_t Frame[] = {
    0x3E,0x3F,0x07,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x03,
    0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x07,0x3F,0x3E,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0xC0,0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0xC0,0x07,0x0F,0x0E,0x0C,
    0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,
    0x0C,0x0C,0x0C,0x0C,0x0C,0x0E,0x0F,0x07,
};

extern const uint8_t OLED_F12x24[][36];

void Draw_Battery_Icon(uint8_t x, uint8_t y, int percent)
{
    uint8_t fill_w;
    uint8_t i;

    OLED_DrawRectangle(x, y, 14, 8, 0);
    OLED_DrawLine(x + 14, y + 2, x + 14, y + 5);

    fill_w = (percent * 10) / 100;
    if (fill_w > 10) fill_w = 10;
    for (i = 0; i < fill_w; i++)
    {
        OLED_DrawLine(x + 2 + i, y + 2, x + 2 + i, y + 5);
    }

    if (percent >= 100)
    {
        OLED_ShowNum(1, 15, 100, 3, 6);
        OLED_ShowString(1, 18, "%", 6);
    }
    else
    {
        OLED_ShowString(1, 15, " ", 6);
        OLED_ShowNum(1, 16, percent, 2, 6);
        OLED_ShowString(1, 18, "%", 6);
    }
}

void Show_Clock_UI(void)
{
    static uint8_t last_alarm_min = 255;
    uint8_t y;
    uint8_t x;

    HAL_RTC_GetTime(&hrtc, &RTC_Time, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &RTC_Date, RTC_FORMAT_BIN);

    if (sys_alarm_enable && (RTC_Time.Hours == sys_alarm_hour) && (RTC_Time.Minutes == sys_alarm_min))
    {
        if (last_alarm_min != RTC_Time.Minutes)
        {
            Alarm_Trigger();
            last_alarm_min = RTC_Time.Minutes;
        }
    }
    else
    {
        if (RTC_Time.Minutes != sys_alarm_min)
        {
            last_alarm_min = 255;
        }
    }

    /* Neu dang reo chuong, ep buc ve man hinh canh bao + hien AS: xxxx */
    if (Alarm_IsRinging())
    {
        OLED_Clear();
        OLED_ShowString(1, 3, "BAO THUC KEU", 8);
        OLED_Printf(2, 4, 8, "%02d:%02d:%02d", RTC_Time.Hours, RTC_Time.Minutes, RTC_Time.Seconds);
        OLED_Printf(3, 3, 8, "AS: %04d", current_light_adc);
        OLED_ShowString(4, 1, "CHE AS / PHIM", 8);
        return;
    }

    OLED_ShowString(1, 1, "20", 6);
    OLED_ShowNum(1, 3, RTC_Date.Year, 2, 6);
    OLED_ShowString(1, 5, "-", 6);
    OLED_ShowNum(1, 6, RTC_Date.Month, 2, 6);
    OLED_ShowString(1, 8, "-", 6);
    OLED_ShowNum(1, 9, RTC_Date.Date, 2, 6);

    if (sys_alarm_enable)
    {
        OLED_ShowImage(70, 0, 8, 8, Mini_Bell_Icon);
    }

    Draw_Battery_Icon(112, 0, Battery_Capacity);

    y = 16;
    x = 16;
    OLED_ShowImage(x, y, 12, 24, OLED_F12x24[RTC_Time.Hours / 10 + 16]);
    OLED_ShowImage(x + 12, y, 12, 24, OLED_F12x24[RTC_Time.Hours % 10 + 16]);
    OLED_ShowImage(x + 24, y, 12, 24, OLED_F12x24[26]);
    OLED_ShowImage(x + 36, y, 12, 24, OLED_F12x24[RTC_Time.Minutes / 10 + 16]);
    OLED_ShowImage(x + 48, y, 12, 24, OLED_F12x24[RTC_Time.Minutes % 10 + 16]);
    OLED_ShowImage(x + 60, y, 12, 24, OLED_F12x24[26]);
    OLED_ShowImage(x + 72, y, 12, 24, OLED_F12x24[RTC_Time.Seconds / 10 + 16]);
    OLED_ShowImage(x + 84, y, 12, 24, OLED_F12x24[RTC_Time.Seconds % 10 + 16]);
}

void Show_Vietnamese_Nav(uint8_t flag)
{
    if (flag == 1)
    {
        OLED_ShowStringWithReverse(8, 2, "> MENU <", 6, 0);
        OLED_ShowStringWithReverse(8, 12, " CAI DAT ", 6, 1);
    }
    else
    {
        OLED_ShowStringWithReverse(8, 2, "  MENU  ", 6, 1);
        OLED_ShowStringWithReverse(8, 12, ">CAI DAT<", 6, 0);
    }
}

int clockflag = 1;

int First_Page_Clock(void)
{
    static uint8_t power_inited = 0;
    if (!power_inited)
    {
        Screen_Power_Init();
        power_inited = 1;
    }

    while (1)
    {
        num = Key_GetNum();

        if (!Screen_IsOn())
        {
            if (num != 0)
            {
                Screen_TurnOn();
            }
            vTaskDelay(pdMS_TO_TICKS(50));
            continue;
        }

        if (num == 1)
        {
            clockflag--;
            if (clockflag <= 0) clockflag = 2;
        }
        else if (num == 2)
        {
            clockflag++;
            if (clockflag >= 3) clockflag = 1;
        }
        else if (num == 3)
        {
            OLED_Clear();
            return clockflag;
        }
        else if (num == 4)
        {
            Screen_TurnOff();
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        OLED_Clear();
        Show_Clock_UI();
        if (!Alarm_IsRinging())
        {
            Show_Vietnamese_Nav(clockflag);
        }
        OLED_Update();

        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void Show_SettingPage_UI(void)
{
    OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 1);
    OLED_ShowString(2, 3, "DAT GIO RTC", 8);
    OLED_ShowString(3, 3, "TEST CHUONG", 8);
}

int setflag = 1;
int Show_SettingPage_UIflag = 0;

int SettingPage(void)
{
    uint8_t setflag_temp;
    Show_SettingPage_UI();
    while (1)
    {
        setflag_temp = 0;
        num = Key_GetNum();
        if (num == 1)
        {
            setflag--;
            if (setflag <= 0) setflag = 3;
        }
        else if (num == 2)
        {
            setflag++;
            if (setflag >= 4) setflag = 1;
        }
        else if (num == 3)
        {
            OLED_Clear();
            setflag_temp = setflag;
        }

        if (setflag_temp == 1)
        {
            setflag = 1;
            return 0;
        }
        else if (setflag_temp == 2)
        {
            Show_SettingPage_UIflag = SetTime();
            RTC_Sync_From_MyRTC();
        }
        else if (setflag_temp == 3)
        {
            Alarm_Trigger();
            return 0;
        }

        if (Show_SettingPage_UIflag)
        {
            Show_SettingPage_UI();
            Show_SettingPage_UIflag = 0;
        }

        OLED_ShowImageWithReverse(0, 0, 16, 16, Return, (setflag == 1) ? 0 : 1);
        OLED_ShowStringWithReverse(2, 3, "DAT GIO RTC", 8, (setflag == 2) ? 0 : 1);
        OLED_ShowStringWithReverse(3, 3, "TEST CHUONG", 8, (setflag == 3) ? 0 : 1);

        OLED_Update();
    }
}

uint8_t pre_selection;
uint8_t target_selection;
uint8_t x_pre = 48;
uint8_t Speed = 8;
uint8_t move_flag;

uint8_t Get_Safe_Index(int8_t index)
{
    while (index < 1) index += 7;
    while (index > 7) index -= 7;
    return index;
}

void Menu_Animation(void)
{
    OLED_Clear();
    OLED_ShowImage(42, 10, 44, 48, Frame);

    if (pre_selection < target_selection)
    {
        if (x_pre >= Speed) x_pre -= Speed;
        else x_pre = 0;

        if (x_pre == 0)
        {
            pre_selection++;
            move_flag = 0;
            x_pre = 48;
            if (pre_selection > 7) pre_selection = 1;
        }
    }

    if (pre_selection > target_selection)
    {
        x_pre += Speed;
        if (x_pre >= 96)
        {
            pre_selection--;
            move_flag = 0;
            x_pre = 48;
            if (pre_selection < 1) pre_selection = 7;
        }
    }

    OLED_ShowImage(x_pre - 48, 16, 32, 32, Menu_Graph[Get_Safe_Index(pre_selection - 1) - 1]);
    OLED_ShowImage(x_pre, 16, 32, 32, Menu_Graph[Get_Safe_Index(pre_selection) - 1]);
    OLED_ShowImage(x_pre + 48, 16, 32, 32, Menu_Graph[Get_Safe_Index(pre_selection + 1) - 1]);
    OLED_ShowImage(x_pre + 96, 16, 32, 32, Menu_Graph[Get_Safe_Index(pre_selection + 2) - 1]);

    OLED_Update();
}

void Set_Selection(uint8_t move_flag_in, uint8_t Pre_Selection, uint8_t Target_Selection)
{
    if (move_flag_in == 1)
    {
        pre_selection = Pre_Selection;
        target_selection = Target_Selection;
    }
    Menu_Animation();
}

uint8_t menu_flag = 1;

void MenuToFunction(void)
{
    uint8_t i;
    uint8_t y_offset;
    for (i = 0; i <= 6; i++)
    {
        OLED_Clear();
        y_offset = i * 8;
        OLED_ShowImage(42, 10 + y_offset, 44, 48, Frame);
        OLED_ShowImage(0, 16 + y_offset, 32, 32, Menu_Graph[Get_Safe_Index(menu_flag - 1) - 1]);
        OLED_ShowImage(48, 16 + y_offset, 32, 32, Menu_Graph[Get_Safe_Index(menu_flag) - 1]);
        OLED_ShowImage(96, 16 + y_offset, 32, 32, Menu_Graph[Get_Safe_Index(menu_flag + 1) - 1]);
        OLED_Update();
        HAL_Delay(10);
    }
    OLED_Clear();
    OLED_Update();
}

int Menu(void)
{
    uint8_t last_selection;
    uint8_t menu_flag_temp;
    pre_selection = menu_flag;
    target_selection = menu_flag;
    x_pre = 48;
    move_flag = 1;
    last_selection = menu_flag;

    while (1)
    {
        menu_flag_temp = 0;
        num = Key_GetNum();
        if (num == 1 || num == 2)
        {
            last_selection = menu_flag;
            move_flag = 1;

            if (num == 1)
            {
                menu_flag--;
                if (menu_flag <= 0) menu_flag = 7;
            }
            else if (num == 2)
            {
                menu_flag++;
                if (menu_flag >= 8) menu_flag = 1;
            }

            if (last_selection == 1 && menu_flag == 7)
            {
                Set_Selection(move_flag, 1, 0);
            }
            else if (last_selection == 7 && menu_flag == 1)
            {
                Set_Selection(move_flag, 7, 8);
            }
            else
            {
                Set_Selection(move_flag, last_selection, menu_flag);
            }
        }
        else if (num == 3)
        {
            OLED_Clear();
            OLED_Update();
            menu_flag_temp = menu_flag;
        }

        if (menu_flag_temp != 0)
        {
            if (menu_flag_temp == 1) return 0;
            return menu_flag_temp;
        }

        if (move_flag)
        {
            Menu_Animation();
        }
    }
}

volatile uint8_t hour = 0, min = 0, sec = 0;
volatile uint8_t start_timing_flag = 0;

void Show_StopWatch_UI(void)
{
    OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 1);
    OLED_ShowNumWithReverse(2, 5, hour, 2, 8, 1);
    OLED_ShowStringWithReverse(2, 7, ":", 8, 1);
    OLED_ShowNumWithReverse(2, 8, min, 2, 8, 1);
    OLED_ShowStringWithReverse(2, 10, ":", 8, 1);
    OLED_ShowNumWithReverse(2, 11, sec, 2, 8, 1);
    OLED_ShowString(4, 1, "CHAY", 8);
    OLED_ShowString(4, 6, "DUNG", 8);
    OLED_ShowString(4, 11, "XOA", 8);
    OLED_Update();
}

void Show_Time_Only(void)
{
    OLED_ShowNumWithReverse(2, 5, hour, 2, 8, 1);
    OLED_ShowStringWithReverse(2, 7, ":", 8, 1);
    OLED_ShowNumWithReverse(2, 8, min, 2, 8, 1);
    OLED_ShowStringWithReverse(2, 10, ":", 8, 1);
    OLED_ShowNumWithReverse(2, 11, sec, 2, 8, 1);
}

void StopWatch_Tick(void)
{
    static uint16_t Count = 0;
    if (start_timing_flag == 1)
    {
        Count++;
        if (Count >= 1000)
        {
            Count = 0;
            sec++;
            if (sec >= 60)
            {
                sec = 0;
                min++;
                if (min >= 60)
                {
                    min = 0;
                    hour++;
                    if (hour > 99) hour = 0;
                }
            }
        }
    }
}

uint8_t stopwatch_flag = 1;

int StopWatch(void)
{
    uint8_t stopwatch_flag_temp;
    Show_StopWatch_UI();
    while (1)
    {
        stopwatch_flag_temp = 0;
        num = Key_GetNum();
        if (num == 1)
        {
            stopwatch_flag--;
            if (stopwatch_flag <= 0) stopwatch_flag = 4;
        }
        else if (num == 2)
        {
            stopwatch_flag++;
            if (stopwatch_flag >= 5) stopwatch_flag = 1;
        }
        else if (num == 3)
        {
            OLED_Clear();
            OLED_Update();
            stopwatch_flag_temp = stopwatch_flag;
        }

        if (stopwatch_flag_temp == 1) return 0;

        Show_Time_Only();
        OLED_ShowImageWithReverse(0, 0, 16, 16, Return, (stopwatch_flag == 1) ? 0 : 1);
        OLED_ShowStringWithReverse(4, 1, "CHAY", 8, (stopwatch_flag == 2) ? 0 : 1);
        OLED_ShowStringWithReverse(4, 6, "DUNG", 8, (stopwatch_flag == 3) ? 0 : 1);
        OLED_ShowStringWithReverse(4, 11, "XOA", 8, (stopwatch_flag == 4) ? 0 : 1);

        if (stopwatch_flag == 2 && num == 3) start_timing_flag = 1;
        if (stopwatch_flag == 3 && num == 3) start_timing_flag = 0;
        if (stopwatch_flag == 4 && num == 3) { start_timing_flag = 0; hour = min = sec = 0; }

        OLED_Update();
    }
}

void Show_LED_UI(void)
{
    OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 1);
    OLED_ShowStringWithReverse(2, 3, "OFF", 12, 1);
    OLED_ShowStringWithReverse(2, 7, "ON", 12, 1);
    OLED_Update();
}

uint8_t led_flag = 1;

int LED(void)
{
    uint8_t led_flag_temp;
    Show_LED_UI();
    while (1)
    {
        led_flag_temp = 0;
        num = Key_GetNum();
        if (num == 1)
        {
            led_flag--;
            if (led_flag <= 0) led_flag = 3;
        }
        else if (num == 2)
        {
            led_flag++;
            if (led_flag >= 4) led_flag = 1;
        }
        else if (num == 3)
        {
            OLED_Clear();
            OLED_Update();
            led_flag_temp = led_flag;
        }

        if (led_flag_temp == 1) return 0;

        switch (led_flag)
        {
            case 1:
                OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 0);
                OLED_ShowStringWithReverse(2, 3, "OFF", 12, 1);
                OLED_ShowStringWithReverse(2, 7, "ON", 12, 1);
                OLED_Update();
                break;
            case 2:
                /* OFF -> PB15 SET (active-low tat) */
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_SET);
                OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 1);
                OLED_ShowStringWithReverse(2, 3, "OFF", 12, 0);
                OLED_ShowStringWithReverse(2, 7, "ON", 12, 1);
                OLED_Update();
                break;
            case 3:
                /* ON -> PB15 RESET (active-low sang) */
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);
                OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 1);
                OLED_ShowStringWithReverse(2, 3, "OFF", 12, 1);
                OLED_ShowStringWithReverse(2, 7, "ON", 12, 0);
                OLED_Update();
                break;
        }
    }
}

int16_t ax, ay, az, gx, gy, gz;
float roll_g, pitch_g, yaw_g;
float roll_a, pitch_a;
float Roll, Pitch, Yaw;
float a = 0.9f;
float Delta_t = 0.005f;
double pi = 3.1415927;

void MPU6050_Calculation(void)
{
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);

    roll_g = Roll + (float)gx * Delta_t;
    pitch_g = Pitch + (float)gy * Delta_t;
    yaw_g = Yaw + (float)gz * Delta_t;

    pitch_a = atan2((-1) * ax, az) * 180 / pi;
    roll_a = atan2(ay, az) * 180 / pi;

    Roll = a * roll_g + (1 - a) * roll_a;
    Pitch = a * pitch_g + (1 - a) * pitch_a;
    Yaw = a * yaw_g;
}

void Show_MPU6050_UI(void)
{
    OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 0);
    OLED_Printf(2, 1, 8, "Roll: %.2f", Roll);
    OLED_Printf(3, 1, 8, "Pitch:%.2f", Pitch);
    OLED_Printf(4, 1, 8, "Yaw:  %.2f", Yaw);
    OLED_Update();
}

int MPU6050(void)
{
    while (1)
    {
        num = Key_GetNum();
        if (num == 3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        OLED_Clear();
        MPU6050_Calculation();
        Show_MPU6050_UI();
    }
}

void Show_Game_UI(void)
{
    OLED_Clear();
    OLED_ShowImageWithReverse(0, 0, 16, 16, Return, 1);
    OLED_ShowString(2, 2, "1. NUT BAM", 8);
    OLED_ShowString(3, 2, "2. CAM BIEN", 8);
    OLED_Update();
}

uint8_t game_flag = 1;

int Game(void)
{
    Show_Game_UI();
    while (1)
    {
        num = Key_GetNum();
        if (num == 1)
        {
            game_flag--;
            if (game_flag < 1) game_flag = 3;
        }
        else if (num == 2)
        {
            game_flag++;
            if (game_flag > 3) game_flag = 1;
        }
        else if (num == 3)
        {
            if (game_flag == 1)
            {
                game_flag = 1;
                OLED_Clear();
                OLED_Update();
                return 0;
            }
            else if (game_flag == 2)
            {
                DinoGame_Pos_Init();
                DinoGame_Animation(0);
                Show_Game_UI();
            }
            else if (game_flag == 3)
            {
                DinoGame_Pos_Init();
                DinoGame_Animation(1);
                Show_Game_UI();
            }
        }

        OLED_Clear();
        OLED_ShowImageWithReverse(0, 0, 16, 16, Return, (game_flag == 1) ? 0 : 1);
        OLED_ShowStringWithReverse(2, 2, "1. NUT BAM", 8, (game_flag == 2) ? 0 : 1);
        OLED_ShowStringWithReverse(3, 2, "2. CAM BIEN", 8, (game_flag == 3) ? 0 : 1);
        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

int Emoji(void)
{
    uint8_t cursor = 1;

    while (1)
    {
        num = Key_GetNum();
        if (num == 1)
        {
            cursor--;
            if (cursor <= 0) cursor = 4;
        }
        else if (num == 2)
        {
            cursor++;
            if (cursor >= 5) cursor = 1;
        }
        else if (num == 3)
        {
            if (cursor == 1)
            {
                OLED_Clear();
                return 0;
            }
            else if (cursor == 2)
            {
                sys_alarm_hour = (sys_alarm_hour + 1) % 24;
            }
            else if (cursor == 3)
            {
                sys_alarm_min = (sys_alarm_min + 1) % 60;
            }
            else if (cursor == 4)
            {
                sys_alarm_enable = !sys_alarm_enable;
            }
        }

        OLED_Clear();
        OLED_ShowImageWithReverse(0, 0, 16, 16, Return, (cursor == 1) ? 0 : 1);
        OLED_ShowString(1, 4, "CAI BAO THUC", 8);

        OLED_ShowString(3, 2, "GIO:", 8);
        OLED_ShowNumWithReverse(3, 7, sys_alarm_hour, 2, 8, (cursor == 2) ? 0 : 1);
        OLED_ShowString(3, 9, ":", 8);
        OLED_ShowNumWithReverse(3, 10, sys_alarm_min, 2, 8, (cursor == 3) ? 0 : 1);

        OLED_ShowString(4, 2, "TRANG THAI:", 8);
        if (sys_alarm_enable)
        {
            OLED_ShowStringWithReverse(4, 14, "BAT", 8, (cursor == 4) ? 0 : 1);
        }
        else
        {
            OLED_ShowStringWithReverse(4, 14, "TAT", 8, (cursor == 4) ? 0 : 1);
        }

        OLED_Update();
        vTaskDelay(pdMS_TO_TICKS(40));
    }
}

void Show_Gradienter_UI(void)
{
    int center_x;
    int center_y;
    int r_big;
    int r_small;
    int max_dist;
    float sensitivity;
    float offset_x;
    float offset_y;
    float dist;

    MPU6050_Calculation();

    center_x = 64;
    center_y = 32;
    r_big = 28;
    r_small = 4;
    max_dist = r_big - r_small;

    sensitivity = 1.0f;
    offset_x = -Roll * sensitivity;
    offset_y = Pitch * sensitivity;

    dist = sqrt(offset_x * offset_x + offset_y * offset_y);

    if (dist > max_dist && dist > 0.001f)
    {
        float scale = max_dist / dist;
        offset_x *= scale;
        offset_y *= scale;
    }

    OLED_DrawCircle(center_x, center_y, r_big, 0);
    OLED_DrawCircle(center_x + (int)offset_x, center_y + (int)offset_y, r_small, 1);
}

int Gradienter(void)
{
    while (1)
    {
        num = Key_GetNum();
        if (num == 3)
        {
            OLED_Clear();
            OLED_Update();
            return 0;
        }
        OLED_Clear();
        Show_Gradienter_UI();
        OLED_Update();
    }
}