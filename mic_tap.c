#include "mic_tap.h"
#include "adc.h"
#include "OLED.h"
#include "Key.h"
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

#define BUZZER_PORT             GPIOB
#define BUZZER_PIN              GPIO_PIN_14

#define LED_PORT                GPIOB
#define LED_PIN                 GPIO_PIN_15

/* Nguon sang di gan nen che tay lech ~200 -> dat 150 de bat chuan */
#define LIGHT_DELTA_THRESHOLD   150
#define LIGHT_CONFIRM_COUNT     4
#define LIGHT_WARMUP_CYCLES     4

static uint8_t is_screen_on = 1;
static uint8_t is_alarm_ringing = 0;

int Battery_Capacity = 100;
static float battery_ema = 100.0f;

/* Mac dinh khi khoi dong la TAT bao thuc */
uint8_t sys_alarm_hour = 7;
uint8_t sys_alarm_min = 0;
uint8_t sys_alarm_enable = 0;

uint16_t current_light_adc = 0;
static uint16_t alarm_base_light = 0;
static uint8_t warmup_cnt = 0;
static uint8_t alarm_base_ready = 0;
static uint8_t light_hit_count = 0;

static void Alarm_Pins_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOB_CLK_ENABLE();

    GPIO_InitStruct.Pin = BUZZER_PIN | LED_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

static uint16_t ADC_Read_Channel(uint32_t Channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = Channel;
    sConfig.Rank = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;

    HAL_ADC_Stop(&hadc1);
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);

    uint16_t adc_val = 0;
    if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
    {
        adc_val = (uint16_t)HAL_ADC_GetValue(&hadc1);
    }
    HAL_ADC_Stop(&hadc1);
    return adc_val;
}

void Screen_TurnOn(void)
{
    is_screen_on = 1;
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x14);
    OLED_WriteCommand(0xAF);
    OLED_Update();
}

void Screen_TurnOff(void)
{
    is_screen_on = 0;
    OLED_Clear();
    OLED_Update();
    OLED_WriteCommand(0x8D);
    OLED_WriteCommand(0x10);
    OLED_WriteCommand(0xAE);
}

void Screen_Toggle(void)
{
    if (is_screen_on) Screen_TurnOff();
    else              Screen_TurnOn();
}

uint8_t Screen_IsOn(void)
{
    return is_screen_on;
}

void Screen_Power_Init(void)
{
    Alarm_Pins_Init();
    is_screen_on = 1;
    is_alarm_ringing = 0;
    alarm_base_ready = 0;
    warmup_cnt = 0;
    light_hit_count = 0;
    OLED_WriteCommand(0xAF);
}

void Alarm_Trigger(void)
{
    is_alarm_ringing = 1;
    alarm_base_ready = 0;
    warmup_cnt = 0;
    light_hit_count = 0;
}

void Alarm_Stop(void)
{
    is_alarm_ringing = 0;
    alarm_base_ready = 0;
    warmup_cnt = 0;
    light_hit_count = 0;
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_SET);
}

uint8_t Alarm_IsRinging(void)
{
    return is_alarm_ringing;
}

static void Alarm_Dismiss_Now(void)
{
    Alarm_Stop();
    sys_alarm_enable = 0;

    OLED_Clear();
    OLED_ShowString(2, 2, "DA TAT BAO THUC", 8);
    OLED_ShowString(3, 4, "NGU TIEP...", 8);
    OLED_Update();
    vTaskDelay(pdMS_TO_TICKS(500));
}

/* Man hinh doc quyen khi bao thuc keu:
   - Nhan phim vat ly bat ky -> tat bao thuc ngay
   - Bat LED PB15 (RESET) de chieu sang cam bien
   - Tat khi up tay che lam lech anh sang >= LIGHT_DELTA_THRESHOLD on dinh */
void Alarm_Ringing_Screen(void)
{
    /* Tat vat ly khi co phim duoc nhan */
    if (Key_GetNum() != 0)
    {
        Alarm_Dismiss_Now();
        return;
    }

    HAL_GPIO_WritePin(LED_PORT, LED_PIN, GPIO_PIN_RESET);

    uint32_t l_sum = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        l_sum += ADC_Read_Channel(ADC_CHANNEL_7);
    }
    current_light_adc = (uint16_t)(l_sum / 16);

    static uint8_t beep = 0;
    beep = !beep;
    HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, beep ? GPIO_PIN_SET : GPIO_PIN_RESET);

    if (!alarm_base_ready)
    {
        warmup_cnt++;
        alarm_base_light = current_light_adc;
        if (warmup_cnt >= LIGHT_WARMUP_CYCLES)
        {
            alarm_base_ready = 1;
            light_hit_count = 0;
        }
        return;
    }

    int32_t diff = (int32_t)current_light_adc - (int32_t)alarm_base_light;
    if (diff < 0) diff = -diff;

    if (diff >= LIGHT_DELTA_THRESHOLD)
    {
        light_hit_count++;
        if (light_hit_count >= LIGHT_CONFIRM_COUNT)
        {
            Alarm_Dismiss_Now();
        }
    }
    else
    {
        if (light_hit_count > 0) light_hit_count--;
    }
}

void Hardware_Sensors_Update(void)
{
    /* 1. Doc bien tro pin tren PA0 (0% - 100%) */
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 16; i++)
    {
        sum += ADC_Read_Channel(ADC_CHANNEL_0);
    }
    uint16_t raw_pot = sum / 16;

    #define POT_FLOOR   100
    #define POT_ROOF    3550

    float calc_cap;
    if (raw_pot <= POT_FLOOR)      calc_cap = 0.0f;
    else if (raw_pot >= POT_ROOF) calc_cap = 100.0f;
    else calc_cap = ((float)(raw_pot - POT_FLOOR) * 100.0f) / (float)(POT_ROOF - POT_FLOOR);

    battery_ema = (0.85f * battery_ema) + (0.15f * calc_cap);
    Battery_Capacity = (int)(battery_ema + 0.5f);
    if (Battery_Capacity > 100) Battery_Capacity = 100;
    if (Battery_Capacity < 0)   Battery_Capacity = 0;

    /* 2. Neu dang reo chuong, goi ham xu ly chuong va cam bien anh sang */
    if (is_alarm_ringing)
    {
        Alarm_Ringing_Screen();
    }
}