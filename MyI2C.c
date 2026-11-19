#include "gpio.h"
#include <stdint.h>

/* Hàm trễ microsecond bắt buộc cho I2C phần mềm ở 72MHz */
static void MyI2C_Delay(void)
{
    for (volatile uint32_t i = 0; i < 20; i++);
}

void MyI2C_W_SCL(uint8_t BitValue)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_10, BitValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
    MyI2C_Delay();
}

void MyI2C_W_SDA(uint8_t BitValue)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, BitValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
    MyI2C_Delay();
}

uint8_t MyI2C_R_SDA(void)
{
    return HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_11);
}

void MyI2C_Init(void)
{
    MX_GPIO_Init();
    MyI2C_W_SCL(1);
    MyI2C_W_SDA(1);
}

void MyI2C_Start(void)
{
    MyI2C_W_SDA(1);
    MyI2C_W_SCL(1);
    MyI2C_Delay();
    MyI2C_W_SDA(0);
    MyI2C_Delay();
    MyI2C_W_SCL(0);
    MyI2C_Delay();
}

void MyI2C_Stop(void)
{
    MyI2C_W_SDA(0);
    MyI2C_Delay();
    MyI2C_W_SCL(1);
    MyI2C_Delay();
    MyI2C_W_SDA(1);
    MyI2C_Delay();
}

void MyI2C_SendByte(uint8_t Byte)
{
    for (uint8_t i = 0; i < 8; i++)
    {
        MyI2C_W_SDA(!!(Byte & (0x80 >> i)));
        MyI2C_W_SCL(1);
        MyI2C_Delay();
        MyI2C_W_SCL(0);
        MyI2C_Delay();
    }
}

uint8_t MyI2C_ReceiveByte(void)
{
    uint8_t Byte = 0x00;
    MyI2C_W_SDA(1);
    for (uint8_t i = 0; i < 8; i++)
    {
        MyI2C_W_SCL(1);
        MyI2C_Delay();
        if (MyI2C_R_SDA()) { Byte |= (0x80 >> i); }
        MyI2C_W_SCL(0);
        MyI2C_Delay();
    }
    return Byte;
}

void MyI2C_SendAck(uint8_t AckBit)
{
    MyI2C_W_SDA(AckBit);
    MyI2C_W_SCL(1);
    MyI2C_Delay();
    MyI2C_W_SCL(0);
    MyI2C_Delay();
}

uint8_t MyI2C_ReceiveAck(void)
{
    MyI2C_W_SDA(1);
    MyI2C_W_SCL(1);
    MyI2C_Delay();
    uint8_t AckBit = MyI2C_R_SDA();
    MyI2C_W_SCL(0);
    MyI2C_Delay();
    return AckBit;
}
