#include "RTC.h"
#include "OLED.h"
#include "Key.h"

const uint8_t Return1[] = {
    0x00,0x00,0x80,0xC0,0xE0,0xF0,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0xF8,0xF8,0x00,
    0x00,0x01,0x03,0x07,0x0F,0x1F,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x03,0x00,
};

RTC_DateTypeDef RTC_Date1;
RTC_TimeTypeDef RTC_Time1;

extern uint8_t MyRTC_Time[];

void Show_SetTime_FirstUI(void)
{
    OLED_Clear();
    OLED_ShowImageWithReverse(0, 0, 16, 16, Return1, 1);
    
    // In nhãn ch? c? d?nh
    OLED_ShowString(2, 1, "NAM  : 20", 8);
    OLED_ShowString(3, 1, "THANG: ", 8);
    OLED_ShowString(4, 1, "NGAY : ", 8);
    
    HAL_RTC_GetDate(&hrtc, &RTC_Date1, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time1, RTC_FORMAT_BIN);
    
    // Ch? in dúng 2 ch? s? giá tr?
    OLED_ShowNum(2, 10, RTC_Date1.Year, 2, 8);
    OLED_ShowNum(3, 8, RTC_Date1.Month, 2, 8); 
    OLED_ShowNum(4, 8, RTC_Date1.Date, 2, 8);
}

void Show_SetTime_SecondUI(void)
{
    OLED_Clear();
    OLED_ShowString(1, 1, "GIO  : ", 8);
    OLED_ShowString(2, 1, "PHUT : ", 8);
    OLED_ShowString(3, 1, "GIAY : ", 8);
    
    HAL_RTC_GetDate(&hrtc, &RTC_Date1, RTC_FORMAT_BIN);
    HAL_RTC_GetTime(&hrtc, &RTC_Time1, RTC_FORMAT_BIN);
    
    OLED_ShowNum(1, 8, RTC_Time1.Hours, 2, 8);
    OLED_ShowNum(2, 8, RTC_Time1.Minutes, 2, 8);
    OLED_ShowNum(3, 8, RTC_Time1.Seconds, 2, 8);
}

uint8_t KeyNum1, KeyNum2, KeyNum3, KeyNum4, KeyNum5, KeyNum6;

int SetYear(void)
{
    while(1)
    {
        KeyNum1 = Key_GetNum();
        if(KeyNum1 == 1)      Change_RTC_Time(0, 1); 
        else if(KeyNum1 == 2) Change_RTC_Time(0, 0); 
        else if(KeyNum1 == 3) return 1;              

        Show_SetTime_FirstUI();
        // Ch? bôi den dúng 2 con s? nam (ví d?: s? 26)
        OLED_ShowNumWithReverse(2, 10, MyRTC_Time[0], 2, 8, 0); 
        OLED_Update();
    }
}

int SetMonth(void)
{
    while(1)
    {
        KeyNum2 = Key_GetNum();
        if(KeyNum2 == 1)      Change_RTC_Time(1, 1); 
        else if(KeyNum2 == 2) Change_RTC_Time(1, 0); 
        else if(KeyNum2 == 3) return 1;

        Show_SetTime_FirstUI();
        // Ch? bôi den dúng 2 s? tháng
        OLED_ShowNumWithReverse(3, 8, MyRTC_Time[1], 2, 8, 0);
        OLED_Update();
    }
}

int SetDay(void)
{
    while(1)
    {
        KeyNum3 = Key_GetNum();
        if(KeyNum3 == 1)      Change_RTC_Time(2, 1); 
        else if(KeyNum3 == 2) Change_RTC_Time(2, 0); 
        else if(KeyNum3 == 3) return 1;

        Show_SetTime_FirstUI();
        // Ch? bôi den dúng 2 s? ngày
        OLED_ShowNumWithReverse(4, 8, MyRTC_Time[2], 2, 8, 0);
        OLED_Update();
    }
}

int SetHour(void)
{
    while(1)
    {
        KeyNum4 = Key_GetNum();
        if(KeyNum4 == 1)      Change_RTC_Time(3, 1); 
        else if(KeyNum4 == 2) Change_RTC_Time(3, 0); 
        else if(KeyNum4 == 3) return 1;

        Show_SetTime_SecondUI(); 
        // Ch? bôi den 2 s? gi?
        OLED_ShowNumWithReverse(1, 8, MyRTC_Time[3], 2, 8, 0);
        OLED_Update();
    }
}

int SetMin(void)
{
    while(1)
    {
        KeyNum5 = Key_GetNum();
        if(KeyNum5 == 1)      Change_RTC_Time(4, 1); 
        else if(KeyNum5 == 2) Change_RTC_Time(4, 0); 
        else if(KeyNum5 == 3) return 1;

        Show_SetTime_SecondUI();
        // Ch? bôi den 2 s? phút
        OLED_ShowNumWithReverse(2, 8, MyRTC_Time[4], 2, 8, 0);
        OLED_Update();
    }
}

int SetSec(void)
{
    while(1)
    {
        KeyNum6 = Key_GetNum();
        if(KeyNum6 == 1)      Change_RTC_Time(5, 1); 
        else if(KeyNum6 == 2) Change_RTC_Time(5, 0); 
        else if(KeyNum6 == 3) return 1;

        Show_SetTime_SecondUI();
        // Ch? bôi den 2 s? giây
        OLED_ShowNumWithReverse(3, 8, MyRTC_Time[5], 2, 8, 0);
        OLED_Update();
    }
}

uint8_t KeyNum;
int set_time_flag = 1;

int SetTime(void)
{
    while(1)
    {
        KeyNum = Key_GetNum();
        uint8_t set_time_flag_temp = 0;
        if(KeyNum == 1)
        {
            set_time_flag--;
            if(set_time_flag <= 0) set_time_flag = 7;
        }
        else if(KeyNum == 2)
        {
            set_time_flag++;
            if(set_time_flag >= 8) set_time_flag = 1;
        }
        else if(KeyNum == 3)
        {
            OLED_Clear();
            set_time_flag_temp = set_time_flag;
        }
        
        if(set_time_flag_temp == 1)
        {
            set_time_flag = 1;
            return 1;
        }
        else if(set_time_flag_temp == 2)
        {
            SetYear();
        }
        else if(set_time_flag_temp == 3)
        {
            SetMonth();
        }
        else if(set_time_flag_temp == 4)
        {
            SetDay();
        }
        else if(set_time_flag_temp == 5)
        {
            SetHour();
        }
        else if(set_time_flag_temp == 6)
        {
            SetMin();
        }
        else if(set_time_flag_temp == 7)
        {
            SetSec();
        }

        if(set_time_flag <= 4)
        {
            Show_SetTime_FirstUI();
            if(set_time_flag == 1) 
            {
                OLED_ShowImageWithReverse(0, 0, 16, 16, Return1, 0);
            }
            else if(set_time_flag == 2) 
            {
                // Ch? bôi den duy nh?t ch? "NAM" (3 ký t?)
                OLED_ShowStringWithReverse(2, 1, "NAM", 8, 0);
            }
            else if(set_time_flag == 3) 
            {
                // Ch? bôi den duy nh?t ch? "THANG" (5 ký t?)
                OLED_ShowStringWithReverse(3, 1, "THANG", 8, 0);
            }
            else if(set_time_flag == 4) 
            {
                // Ch? bôi den duy nh?t ch? "NGAY" (4 ký t?)
                OLED_ShowStringWithReverse(4, 1, "NGAY", 8, 0);
            }
        }
        else
        {
            Show_SetTime_SecondUI();
            if(set_time_flag == 5) 
            {
                // Ch? bôi den ch? "GIO"
                OLED_ShowStringWithReverse(1, 1, "GIO", 8, 0);
            }
            else if(set_time_flag == 6) 
            {
                // Ch? bôi den ch? "PHUT"
                OLED_ShowStringWithReverse(2, 1, "PHUT", 8, 0);
            }
            else if(set_time_flag == 7) 
            {
                // Ch? bôi den ch? "GIAY"
                OLED_ShowStringWithReverse(3, 1, "GIAY", 8, 0);
            }
        }
        OLED_Update();
    }
}