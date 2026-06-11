/*
 * ssd1306.h
 *
 *  Created on: 2026年5月29日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_SSD1306_H_
#define APPLICATION_USER_CORE_INC_SSD1306_H_
#include "main.h"
#include <stdint.h>

// I2C 句柄（由 CubeMX 生成，此处外部声明）
extern I2C_HandleTypeDef hi2c1;

// OLED 屏幕参数
#define SSD1306_I2C_ADDR        0x78    // 写地址（常见，若为 0x7A 则修改）
#define SSD1306_WIDTH           128
#define SSD1306_HEIGHT          64

// 初始化 OLED
void SSD1306_Init(void);

// 清屏（全黑）
void SSD1306_Clear(void);

// 更新显示（将显存内容发送到屏幕）
void SSD1306_Update(void);

// 设置光标位置（像素坐标）
void SSD1306_GotoXY(uint16_t x, uint16_t y);

// 显示字符串（当前光标位置）
void SSD1306_Puts(const char *str);

// 显示数字（十进制）
void SSD1306_PrintNumber(int32_t num);

// 显示浮点数（一位小数）
void SSD1306_PrintFloat(float num);



#endif /* APPLICATION_USER_CORE_INC_SSD1306_H_ */
