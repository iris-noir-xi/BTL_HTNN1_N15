#include "OLED.h"
#include "OLED_Font.h"
#include "gpio.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <stdlib.h>

#define OLED_W_SCL(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define OLED_W_SDA(x) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_9, (x) ? GPIO_PIN_SET : GPIO_PIN_RESET)

uint8_t OLED_Buffer[8][128]; // Buffer 8 trang, mỗi trang 128 cột
static uint8_t current_page = 0;
static uint8_t current_col = 0;

/* Hàm tạo trễ ngắn để chuẩn hóa sóng I2C ở xung nhịp 72MHz */
static void OLED_I2C_Delay(void)
{
    for (volatile uint32_t i = 0; i < 20; i++);
}

/* Khởi tạo chân I2C */
void OLED_I2C_Init(void)
{
    MX_GPIO_Init();
    OLED_W_SCL(1);
    OLED_W_SDA(1);
}

void OLED_I2C_Start(void)
{
    OLED_W_SDA(1);
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(0);
    OLED_I2C_Delay();
}

void OLED_I2C_Stop(void)
{
    OLED_W_SDA(0);
    OLED_I2C_Delay();
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SDA(1);
    OLED_I2C_Delay();
}

void OLED_I2C_SendByte(uint8_t Byte)
{
    uint8_t i;
    for (i = 0; i < 8; i++)
    {
        OLED_W_SDA((Byte & (0x80 >> i)) ? 1 : 0);
        OLED_I2C_Delay();
        OLED_W_SCL(1);
        OLED_I2C_Delay();
        OLED_W_SCL(0);
        OLED_I2C_Delay();
    }
    // Xung thứ 9 nhả ACK
    OLED_W_SDA(1);
    OLED_I2C_Delay();
    OLED_W_SCL(1);
    OLED_I2C_Delay();
    OLED_W_SCL(0);
    OLED_I2C_Delay();
}

void OLED_WriteCommand(uint8_t Command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(0x78); // Địa chỉ I2C mặc định
    OLED_I2C_SendByte(0x00); // Gửi lệnh
    OLED_I2C_SendByte(Command); 
    OLED_I2C_Stop();
}

void OLED_Update(void)
{
    uint8_t i, n;
    for (i = 0; i < 8; i++)
    {
        OLED_WriteCommand(0xB0 + i); // Set Page
        OLED_WriteCommand(0x02);     // Dịch cột 2 pixel cho chuẩn chip SH1106 1.3"
        OLED_WriteCommand(0x10);     // Set Column High
        
        OLED_I2C_Start();
        OLED_I2C_SendByte(0x78);
        OLED_I2C_SendByte(0x40);     // Chế độ dữ liệu
        for (n = 0; n < 128; n++)
        {
            OLED_I2C_SendByte(OLED_Buffer[i][n]);
        }
        OLED_I2C_Stop();
    }
}

void OLED_Refresh(void)
{
    OLED_Update();
}

void OLED_WriteData(uint8_t Data)
{
    if (current_page < 8 && current_col < 128) {
        OLED_Buffer[current_page][current_col] = Data;
    }
    current_col++;
    if (current_col >= 128) {
        current_col = 0;
    }
}

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
    if (Y < 8) current_page = Y;
    if (X < 128) current_col = X;
}

void OLED_Clear(void)
{  
    memset(OLED_Buffer, 0, sizeof(OLED_Buffer));
    current_page = 0;
    current_col = 0;
}

void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;
    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
            {
                OLED_Buffer[j / 8][i] &= ~(0x01 << (j % 8));
            }
        }
    }
}

void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height)
{
    int16_t i, j;
    for (j = Y; j < Y + Height; j++)
    {
        for (i = X; i < X + Width; i++)
        {
            if (i >= 0 && i <= 127 && j >= 0 && j <= 63)
            {
                OLED_Buffer[j / 8][i] ^= (0x01 << (j % 8));
            }
        }
    }
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char, uint8_t FontSize)
{    
    uint8_t i;
    if(FontSize == 8)
    {
        OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);        
        for (i = 0; i < 8; i++) {
            OLED_WriteData(OLED_F8x16[Char - ' '][i]);            
        }
        OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);    
        for (i = 0; i < 8; i++) {
            OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);       
        }
    }
    else if(FontSize == 6)
    {
        OLED_SetCursor(Line - 1, (Column - 1) * 6);              
        for (i = 0; i < 6; i++) {
            OLED_WriteData(OLED_F6x8[Char - ' '][i]);             
        }
    }
    else if(FontSize == 12)
    {
        OLED_SetCursor((Line - 1) * 3, (Column - 1) * 12);       
        for (i = 0; i < 12; i++) {
            OLED_WriteData(OLED_F12x24[Char - ' '][i]);          
        }
        OLED_SetCursor((Line - 1) * 3 + 1, (Column - 1) * 12);   
        for (i = 0; i < 12; i++) {
            OLED_WriteData(OLED_F12x24[Char - ' '][i + 12]);     
        }
        OLED_SetCursor((Line - 1) * 3 + 2, (Column - 1) * 12);   
        for (i = 0; i < 12; i++) {
            OLED_WriteData(OLED_F12x24[Char - ' '][i + 24]);     
        }
    }
}

uint8_t OLED_ShowCharWithReverse(uint8_t Line, uint8_t Column, char Char, uint8_t FontSize, uint8_t mode)
{    
    uint8_t i, temp;
    if(FontSize == 8)
    {
        OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
        for (i = 0; i < 8; i++) {
            temp = OLED_F8x16[Char - ' '][i];
            OLED_WriteData(mode ? temp : ~temp); 
        }
        OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
        for (i = 0; i < 8; i++) {
            temp = OLED_F8x16[Char - ' '][i + 8];
            OLED_WriteData(mode ? temp : ~temp);
        }
    }
    else if(FontSize == 6)
    {
        OLED_SetCursor(Line - 1, (Column - 1) * 6);
        for (i = 0; i < 6; i++) {
            temp = OLED_F6x8[Char - ' '][i];
            OLED_WriteData(mode ? temp : ~temp);
        }
    }
    else if(FontSize == 12)
    {
        OLED_SetCursor((Line - 1) * 3, (Column - 1) * 12);
        for (i = 0; i < 12; i++) {
            temp = OLED_F12x24[Char - ' '][i];
            OLED_WriteData(mode ? temp : ~temp);
        }
        OLED_SetCursor((Line - 1) * 3 + 1, (Column - 1) * 12);
        for (i = 0; i < 12; i++) {
            temp = OLED_F12x24[Char - ' '][i + 12];
            OLED_WriteData(mode ? temp : ~temp);
        }
        OLED_SetCursor((Line - 1) * 3 + 2, (Column - 1) * 12);
        for (i = 0; i < 12; i++) {
            temp = OLED_F12x24[Char - ' '][i + 24];
            OLED_WriteData(mode ? temp : ~temp);
        }
    }
    return mode;
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++) {
        OLED_ShowChar(Line, Column + i, String[i], FontSize);
    }
}

uint8_t OLED_ShowStringWithReverse(uint8_t Line, uint8_t Column, char *String, uint8_t FontSize, uint8_t mode)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++) {
        OLED_ShowCharWithReverse(Line, Column + i, String[i], FontSize, mode);
    }
    return mode;
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;
    while (Y--) {
        Result *= X;
    }
    return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++) {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0) {
        OLED_ShowChar(Line, Column, '+', FontSize);
        Number1 = Number;
    } else {
        OLED_ShowChar(Line, Column, '-', FontSize);
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++) {
        OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++) {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10) {
            OLED_ShowChar(Line, Column + i, SingleNumber + '0', FontSize);
        } else {
            OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A', FontSize);
        }
    }
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++) {
        OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
    }
}

uint8_t OLED_ShowNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
    uint8_t i;
    for (i = 0; i < Length; i++) {
        OLED_ShowCharWithReverse(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize, mode);
    }
    return mode;
}

uint8_t OLED_ShowSignedNumWithReverse(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
    uint8_t i;
    uint32_t Number1;
    if (Number >= 0) {
        OLED_ShowCharWithReverse(Line, Column, '+', FontSize, mode);
        Number1 = Number;
    } else {
        OLED_ShowCharWithReverse(Line, Column, '-', FontSize, mode);
        Number1 = -Number;
    }
    for (i = 0; i < Length; i++) {
        OLED_ShowCharWithReverse(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize, mode);
    }
    return mode;
}

uint8_t OLED_ShowHexNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++) {
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        if (SingleNumber < 10) {
            OLED_ShowCharWithReverse(Line, Column + i, SingleNumber + '0', FontSize, mode);
        } else {
            OLED_ShowCharWithReverse(Line, Column + i, SingleNumber - 10 + 'A', FontSize, mode);
        }
    }
    return mode;
}

uint8_t OLED_ShowBinNumWithReverse(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length, uint8_t FontSize, uint8_t mode)
{
    uint8_t i;
    for (i = 0; i < Length; i++) {
        OLED_ShowCharWithReverse(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize, mode);
    }
    return mode;
}

/* KHỞI TẠO MÀN HÌNH 1.3 INCH SH1106 CHUẨN */
void OLED_Init(void)
{
    uint32_t i, j;
    for (i = 0; i < 1000; i++) {
        for (j = 0; j < 1000; j++); // Trễ khởi động
    }

    OLED_I2C_Init();

    OLED_WriteCommand(0xAE); // Tắt hiển thị
    OLED_WriteCommand(0x02); // Set low column address
    OLED_WriteCommand(0x10); // Set high column address
    OLED_WriteCommand(0x40); // Set start line
    OLED_WriteCommand(0xB0); // Set page address
    OLED_WriteCommand(0x81); // Chỉnh độ sáng
    OLED_WriteCommand(0xCF);
    OLED_WriteCommand(0xA1); // Set segment remap
    OLED_WriteCommand(0xC8); // Set COM scan direction
    OLED_WriteCommand(0xA6); // Hiển thị thuận
    OLED_WriteCommand(0xA8); // Multiplex ratio
    OLED_WriteCommand(0x3F);
    OLED_WriteCommand(0xD3); // Display offset
    OLED_WriteCommand(0x00);
    OLED_WriteCommand(0xD5); // Display clock
    OLED_WriteCommand(0x80);
    OLED_WriteCommand(0xD9); // Pre-charge
    OLED_WriteCommand(0x22);
    OLED_WriteCommand(0xDA); // COM pin HW config
    OLED_WriteCommand(0x12);
    OLED_WriteCommand(0xDB); // VCOMH
    OLED_WriteCommand(0x40);

    /* BẬT MẠCH BƠM ÁP (CHARGE PUMP) CHO SH1106 */
    OLED_WriteCommand(0xAD);
    OLED_WriteCommand(0x8B);

    OLED_WriteCommand(0xAF); // Bật màn hình

    OLED_Clear();
    OLED_Update();
}

uint8_t OLED_ShowCN(uint8_t Line, uint8_t Column, uint8_t Num, uint8_t mode)  
{     
    uint8_t i;
    uint8_t page_start = (Line - 1) * 2;   
    uint8_t col_start = (Column - 1) * 16; 

    OLED_SetCursor(page_start, col_start);
    for (i = 0; i < 16; i++) {
        uint8_t data = OLED_F10x16[Num][i];
        OLED_WriteData(mode ? data : ~data); 
    }

    OLED_SetCursor(page_start + 1, col_start);
    for (i = 16; i < 32; i++) {
        uint8_t data = OLED_F10x16[Num][i];
        OLED_WriteData(mode ? data : ~data); 
    }
    return mode;
}

void OLED_DrawPoint(uint8_t x, uint8_t y, uint8_t mode)
{
    if (x >= 128 || y >= 64) return;
    uint8_t page = y / 8;
    uint8_t bit_pos = y % 8;
    if (mode) {
        OLED_Buffer[page][x] |= (1 << bit_pos);
    } else {
        OLED_Buffer[page][x] &= ~(1 << bit_pos);
    }
}

uint8_t OLED_ShowBMP(uint8_t x, uint8_t y, uint8_t width, uint8_t height, const uint8_t *bmp, uint8_t mode) 
{
    uint8_t page_end = y + height / 8;
    uint8_t col_end = x + width;
    uint16_t index = 0;
    for (uint8_t page = y; page < page_end; page++) {
        OLED_SetCursor(page, x);
        for (uint8_t col = x; col < col_end; col++) {
            uint8_t data = bmp[index++];
            OLED_WriteData(mode ? data : ~data); 
        }
    }
    return mode;
}

void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image)
{
    uint8_t i = 0, j = 0;
    int16_t Page, Shift;
    for (j = 0; j < ((Height - 1) / 8 + 1); j++) {
        for (i = 0; i < Width; i++) {
            int16_t screen_x = X + i;
            if (screen_x >= 0 && screen_x <= 127) {
                Page = Y / 8;
                Shift = Y % 8;
                uint16_t image_index = j * Width + i;
                if (Page + j >= 0 && Page + j <= 7) {
                    if (image_index < ((Height * Width) / 8)) {
                        OLED_Buffer[Page + j][screen_x] |= Image[image_index] << Shift;
                    }
                }
                if (Page + j + 1 >= 0 && Page + j + 1 <= 7) {                    
                    if (image_index < ((Height * Width) / 8)) {
                        OLED_Buffer[Page + j + 1][screen_x] |= Image[image_index] >> (8 - Shift);
                    }
                }
            }
        }
    }
}

uint8_t OLED_ShowImageWithReverse(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image, uint8_t mode)
{
    uint8_t i = 0, j = 0;
    int16_t Page, Shift;
    for (j = 0; j < ((Height - 1) / 8 + 1); j++) {
        for (i = 0; i < Width; i++) {
            int16_t screen_x = X + i;
            if (screen_x >= 0 && screen_x <= 127) {
                Page = Y / 8;
                Shift = Y % 8;
                uint16_t image_index = j * Width + i;
                uint8_t current_byte = 0;
                if (image_index < ((Height * Width) / 8)) {
                    current_byte = Image[image_index];
                }
                if (mode == 0) {
                    current_byte = ~current_byte;
                }
                if (Page + j >= 0 && Page + j <= 7) {
                    OLED_Buffer[Page + j][screen_x] &= ~(0xFF << Shift);
                    OLED_Buffer[Page + j][screen_x] |= (current_byte << Shift);
                }
                if (Page + j + 1 >= 0 && Page + j + 1 <= 7) {                    
                    OLED_Buffer[Page + j + 1][screen_x] &= ~(0xFF >> (8 - Shift));
                    OLED_Buffer[Page + j + 1][screen_x] |= (current_byte >> (8 - Shift));
                }
            }
        }
    }
    return mode;
}

void OLED_Printf(uint8_t Line, uint8_t Column, uint8_t FontSize, char *format, ...)
{
    char String[256];
    va_list arg;
    uint16_t i = 0;
    int16_t X_Pixel, Y_Pixel;
    
    if (FontSize == 8) {
        X_Pixel = (Column - 1) * 8;
        Y_Pixel = (Line - 1) * 16;
    } else {
        X_Pixel = (Column - 1) * 6;
        Y_Pixel = (Line - 1) * 8;
    }

    va_start(arg, format);
    vsprintf(String, format, arg);
    va_end(arg);

    while (String[i] != '\0') {
        if (String[i] == '\n') {
            Y_Pixel += (FontSize == 8) ? 16 : 8;
            X_Pixel = (Column - 1) * (FontSize == 8 ? 8 : 6);
            i++;
            continue;
        }
        if ((uint8_t)String[i] > 127) {
            i += 2;
            continue; 
        }

        const uint8_t *fontData;
        uint8_t w, h;
        uint8_t cx, cy;
        
        if (FontSize == 8) {
            fontData = OLED_F8x16[String[i] - ' '];
            w = 8; h = 16;
        } else {
            fontData = OLED_F6x8[String[i] - ' '];
            w = 6; h = 8;
        }

        if (X_Pixel + w > 128) break;

        for (cx = 0; cx < w; cx++) {
            uint16_t colData = (h == 16) ? (fontData[cx] | (fontData[cx + 8] << 8)) : fontData[cx];
            for (cy = 0; cy < h; cy++) {
                if (Y_Pixel + cy >= 64) break; 
                if ((colData >> cy) & 0x01) OLED_DrawPoint(X_Pixel + cx, Y_Pixel + cy, 1);
                else OLED_DrawPoint(X_Pixel + cx, Y_Pixel + cy, 0);
            }
        }
        X_Pixel += w;
        i++;
    }
}

uint8_t OLED_GetPoint(int16_t X, int16_t Y)
{
    if (X < 0 || X > 127 || Y < 0 || Y > 63) return 0;
    return (OLED_Buffer[Y / 8][X] >> (Y % 8)) & 0x01;
}

void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
    int16_t x = X0, y = Y0;
    int16_t dx = abs(X1 - X0), dy = abs(Y1 - Y0);
    int8_t sx = (X0 < X1) ? 1 : -1;
    int8_t sy = (Y0 < Y1) ? 1 : -1;
    int16_t err = dx - dy, e2;

    while (1) {
        OLED_DrawPoint(x, y, 1);
        if (x == X1 && y == Y1) break;
        e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x += sx; }
        if (e2 < dx) { err += dx; y += sy; }
    }
}

void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled)
{
    if (IsFilled) {
        uint8_t i;
        for (i = 0; i < Height; i++) {
            OLED_DrawLine(X, Y + i, X + Width - 1, Y + i);
        }
    } else {
        OLED_DrawLine(X, Y, X + Width - 1, Y);
        OLED_DrawLine(X, Y + Height - 1, X + Width - 1, Y + Height - 1);
        OLED_DrawLine(X, Y, X, Y + Height - 1);
        OLED_DrawLine(X + Width - 1, Y, X + Width - 1, Y + Height - 1);
    }
}

void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled)
{
    int16_t x = 0, y = Radius;
    int16_t d = 3 - 2 * Radius;

    while (x <= y) {
        if (IsFilled) {
            OLED_DrawLine(X - x, Y + y, X + x, Y + y);
            OLED_DrawLine(X - y, Y + x, X + y, Y + x);
            OLED_DrawLine(X - x, Y - y, X + x, Y - y);
            OLED_DrawLine(X - y, Y - x, X + y, Y - x);
        } else {
            OLED_DrawPoint(X + x, Y + y, 1);
            OLED_DrawPoint(X - x, Y + y, 1);
            OLED_DrawPoint(X + x, Y - y, 1);
            OLED_DrawPoint(X - x, Y - y, 1);
            OLED_DrawPoint(X + y, Y + x, 1);
            OLED_DrawPoint(X - y, Y + x, 1);
            OLED_DrawPoint(X + y, Y - x, 1);
            OLED_DrawPoint(X - y, Y - x, 1);
        }
        if (d < 0) d = d + 4 * x + 6;
        else {
            d = d + 4 * (x - y) + 10;
            y--;
        }
        x++;
    }
}

void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
    if (IsFilled) {
        int16_t minX = X0 < X1 ? (X0 < X2 ? X0 : X2) : (X1 < X2 ? X1 : X2);
        int16_t maxX = X0 > X1 ? (X0 > X2 ? X0 : X2) : (X1 > X2 ? X1 : X2);
        int16_t minY = Y0 < Y1 ? (Y0 < Y2 ? Y0 : Y2) : (Y1 < Y2 ? Y1 : Y2);
        int16_t maxY = Y0 > Y1 ? (Y0 > Y2 ? Y0 : Y2) : (Y1 > Y2 ? Y1 : Y2);

        if (minX < 0) minX = 0; if (maxX > 127) maxX = 127;
        if (minY < 0) minY = 0; if (maxY > 63) maxY = 63;

        for (int16_t y = minY; y <= maxY; y++) {
            for (int16_t x = minX; x <= maxX; x++) {
                int16_t edge0 = (X1 - X0) * (y - Y0) - (Y1 - Y0) * (x - X0);
                int16_t edge1 = (X2 - X1) * (y - Y1) - (Y2 - Y1) * (x - X1);
                int16_t edge2 = (X0 - X2) * (y - Y2) - (Y0 - Y2) * (x - X2);

                if ((edge0 >= 0 && edge1 >= 0 && edge2 >= 0) || 
                    (edge0 <= 0 && edge1 <= 0 && edge2 <= 0)) {
                    OLED_DrawPoint(x, y, 1);
                }
            }
        }
    } else {
        OLED_DrawLine(X0, Y0, X1, Y1);
        OLED_DrawLine(X1, Y1, X2, Y2);
        OLED_DrawLine(X2, Y2, X0, Y0);
    }
}

void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled)
{
    int16_t x = 0, y = B;
    int32_t a2 = A * A, b2 = B * B;
    int32_t d = b2 - a2 * B + a2 / 4;

    while (b2 * x <= a2 * y) {
        if (IsFilled) {
            OLED_DrawLine(X - x, Y + y, X + x, Y + y);
            OLED_DrawLine(X - x, Y - y, X + x, Y - y);
        } else {
            OLED_DrawPoint(X + x, Y + y, 1);
            OLED_DrawPoint(X - x, Y + y, 1);
            OLED_DrawPoint(X + x, Y - y, 1);
            OLED_DrawPoint(X - x, Y - y, 1);
        }
        if (d < 0) d += b2 * (2 * x + 3);
        else { d += b2 * (2 * x + 3) + a2 * (-2 * y + 2); y--; }
        x++;
    }

    d = b2 * (x * x + x) + a2 * (y * y - y) - a2 * b2 / 4 + (b2 + a2) / 2;
    while (y >= 0) {
        if (IsFilled) {
            OLED_DrawLine(X - x, Y + y, X + x, Y + y);
            OLED_DrawLine(X - x, Y - y, X + x, Y - y);
        } else {
            OLED_DrawPoint(X + x, Y + y, 1);
            OLED_DrawPoint(X - x, Y + y, 1);
            OLED_DrawPoint(X + x, Y - y, 1);
            OLED_DrawPoint(X - x, Y - y, 1);
        }
        if (d < 0) { d += b2 * (2 * x + 2) + a2 * (-2 * y + 3); x++; }
        else d += a2 * (-2 * y + 3);
        y--;
    }
}

void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    float angle;
    int16_t x_curr, y_curr, x_pre, y_pre;
    uint8_t first_point = 1;
    #ifndef M_PI
    #define M_PI 3.14159265358979323846
    #endif

    for (int16_t i = StartAngle; i <= EndAngle; i++) {
        angle = (float)i * M_PI / 180.0f;
        x_curr = X + (int16_t)(Radius * cos(angle));
        y_curr = Y + (int16_t)(Radius * sin(angle));

        if (IsFilled) {
            OLED_DrawLine(X, Y, x_curr, y_curr);
        } else {
            if (!first_point) {
                OLED_DrawLine(x_pre, y_pre, x_curr, y_curr);
            } else {
                OLED_DrawPoint(x_curr, y_curr, 1);
                first_point = 0;
            }
            x_pre = x_curr;
            y_pre = y_curr;
        }
    }
}

void OLED_ShowCharPixel(int16_t X, int16_t Y, char Char)
{
    uint8_t i, b;
    uint8_t c = Char - ' ';
    for (i = 0; i < 6; i++) {
        uint8_t temp = OLED_F6x8[c][i];
        for (b = 0; b < 8; b++) {
            if ((X + i) > 127 || (Y + b) > 63) continue;
            if (temp & (1 << b)) OLED_DrawPoint(X + i, Y + b, 1);
            else OLED_DrawPoint(X + i, Y + b, 0);
        }
    }
}

void OLED_ShowNumPixel(int16_t X, int16_t Y, uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i++) {
        uint8_t single_num = Number / OLED_Pow(10, Length - i - 1) % 10;
        OLED_ShowCharPixel(X + (i * 6), Y, single_num + '0');
    }
}
