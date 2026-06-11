/*
 * mq2.c
 *
 *  Created on: 2026年5月29日
 *      Author:dd
 */


#include "mq2.h"
#include <math.h>

// 外部分压电阻值 (单位 kΩ)
#define R1         5.1f   // 上分压电阻
#define R2        10.0f   // 下分压电阻
#define VREF       3.3f    // ADC 参考电压 (STM32F103 通常为 3.3V)
#define ADC_MAX    4096.0f // 12位 ADC 最大值

// MQ-2 模块内部负载电阻 (单位 kΩ，常见为 5kΩ？用户代码中使用 0.5，这里沿用 0.5kΩ)
#define RL         0.5f

// 浓度公式常数（根据数据手册，此处沿用用户之前经验值）
#define CONST_A    11.5428f
#define CONST_B    2.0f
#define CONST_C    0.6549f
#define CONST_D    100.0f

/**
 * @brief 初始化 MQ-2 传感器（ADC 已在 CubeMX 中初始化，本函数仅做预热提示）
 * @note   MQ-2 需要预热几分钟才能稳定，建议在首次读取前延时等待。
 */
void MQ2_Init(void)
{
    // CubeMX 已经完成 ADC 初始化，此处可以空实现或打印预热提示
    // 如果需要，可以在此处加入预热延时（例如 HAL_Delay(60000)）
}

/**
 * @brief 获取 ADC 原始值（阻塞式单次转换）
 * @return ADC 值 (0~4095)
 */
uint16_t MQ2_GetAdcValue(void)
{
    uint16_t adc_value = 0;
    // 启动 ADC 转换
    HAL_ADC_Start(&hadc1);
    // 等待转换完成（超时 100ms）
    if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK)
    {
        adc_value = HAL_ADC_GetValue(&hadc1);
    }
    // 停止 ADC（如果不需要连续转换）
    HAL_ADC_Stop(&hadc1);
    return adc_value;
}

/**
 * @brief 获取气体浓度（多次采样取平均，分压还原，公式计算）
 * @return 气体浓度值（单位取决于公式标定，通常为 ppm）
 */
float MQ2_GetGasConcentration(void)
{
    #define SAMPLE_COUNT  10
    uint32_t sum = 0;
    uint16_t adc_avg;
    float v_pa1;       // PA1 引脚实际测得的电压 (0~3.3V)
    float v_ao;        // MQ-2 AO 引脚真实电压 (0~5V)
    float RS;          // 传感器电阻 (kΩ)
    float concentration;

    // 1. 采样 SAMPLE_COUNT 次取平均
    for (int i = 0; i < SAMPLE_COUNT; i++)
    {
        sum += MQ2_GetAdcValue();
        // 简单延时，避免连续采样过快（可改用 HAL_Delay，但注意 RTOS 调度）
        HAL_Delay(5);   // 5ms 间隔
    }
    adc_avg = (uint16_t)(sum / SAMPLE_COUNT);

    // 2. 计算 PA1 引脚电压 (0~3.3V)
    v_pa1 = (float)adc_avg * (VREF / ADC_MAX);

    // 3. 还原 MQ-2 AO 引脚真实电压（经过分压电路）
    //    分压公式：V_ao = V_pa1 * (R1 + R2) / R2
    v_ao = v_pa1 * ((R1 + R2) / R2);

    // 4. 计算传感器电阻 RS
    //    电路：V_ao = 5.0V * (RL / (RS + RL))  =>  RS = RL * (5.0 - V_ao) / V_ao
    RS = RL * (5.0f - v_ao) / v_ao;

    // 5. 根据经验公式计算浓度 (ppm)
    //    公式来源：用户原代码 gas_concentration = pow(11.5428 * 2 / RS, 0.6549) * 100
    concentration = pow((CONST_A * CONST_B) / RS, CONST_C) * CONST_D;

    return concentration;
}

/**
 * @brief 检测气体浓度是否超过阈值
 * @param threshold 浓度阈值（单位与 MQ2_GetGasConcentration 返回值一致）
 * @return 1:超过阈值; 0:未超过
 */
uint8_t MQ2_IsGasDetected(uint16_t threshold)
{
    float gas_value = MQ2_GetGasConcentration();
    return (gas_value > threshold) ? 1 : 0;
}
