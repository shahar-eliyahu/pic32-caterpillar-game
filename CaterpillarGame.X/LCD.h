#pragma once

#define LCD_FIRST_ROW 0x80
#define LCD_SECOND_ROW 0xC0
#define LCD_COLUMNS 16
void busy();
void LCD_setControl(char command);
void LCD_setData(char command);
void LCD_Display(char* str  , int len, int shift , int row);
void LCD_Init();