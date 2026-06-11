/*
 * mq2.h
 *
 *  Created on: 2026年5月29日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_MQ2_H_
#define APPLICATION_USER_CORE_MQ2_H_

#include "main.h"
#include <stdint.h>

// ADC 句柄（在 main.c 或 adc.c 中已定义，此处外部声明）
extern ADC_HandleTypeDef hadc1;

// 初始化 MQ-2 传感器（ADC 已在 CubeMX 中初始化，本函数可留空或做预热提示）
void MQ2_Init(void);

// 获取原始 ADC 值 (0~4095)
uint16_t MQ2_GetAdcValue(void);

// 获取气体浓度（经过分压还原、公式计算），返回值单位：ppm 或自定义百分比（根据公式）
float MQ2_GetGasConcentration(void);

// 检测是否超过阈值（简单封装）
uint8_t MQ2_IsGasDetected(uint16_t threshold);

#endif /* APPLICATION_USER_CORE_MQ2_H_ */
