/*
 * ringbuffer.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_RINGBUFFER_H_
#define APPLICATION_USER_CORE_INC_RINGBUFFER_H_

#include "main.h"

// 定义日志缓存最大容量（文档要求 100 条）
#define LOG_MAX_RECORDS 100

// 统一的传感器数据封装 [cite: 6]
// (请将原来 freertos.c 里的 SensorData_t 删掉，移到这里统一管理)
typedef struct {
    float temperature;
    float humidity;
    float smoke;
    uint32_t timestamp; // 使用系统 tick 作为时间戳
} SensorData_t;

// 环形缓冲区结构体
typedef struct {
    SensorData_t buffer[LOG_MAX_RECORDS];
    uint16_t head;  // 写指针
    uint16_t tail;  // 读指针
    uint16_t count; // 当前已有数据量
} SensorLogBuffer_t;

// 外部调用的 API
void RingBuffer_Init(void);
void RingBuffer_Write(SensorData_t *newData);

#endif /* APPLICATION_USER_CORE_INC_RINGBUFFER_H_ */
