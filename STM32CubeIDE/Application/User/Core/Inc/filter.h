/*
 * filter.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_FILTER_H_
#define APPLICATION_USER_CORE_INC_FILTER_H_

#include "main.h"

// ================= 1. 一阶低通滤波器 (专治 MQ-2 模拟量毛刺) =================
typedef struct {
    float last_out;  // 上一次的输出值
    float alpha;     // 滤波系数 (0~1，越小越平滑但响应越慢)
    uint8_t is_init; // 是否已经初始化过第一帧
} EmaFilter_t;

void EmaFilter_Init(EmaFilter_t *f, float alpha);
float EmaFilter_Process(EmaFilter_t *f, float new_val);


// ================= 2. 中值平均滤波器 (专治 DHT11 突发飞点) =================
#define MEDIAN_SAMPLE_COUNT 5

typedef struct {
    float buf[MEDIAN_SAMPLE_COUNT];
    uint8_t index;
    uint8_t is_full;
} MedianFilter_t;

void MedianFilter_Init(MedianFilter_t *f);
float MedianFilter_Process(MedianFilter_t *f, float new_val);

#endif /* APPLICATION_USER_CORE_INC_FILTER_H_ */
