/*
 * ringbuffer.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "ringbuffer.h"
#include "FreeRTOS.h"
#include "semphr.h"

// 实例化环形缓冲区
static SensorLogBuffer_t g_LogBuffer;

// 引入即将要在 freertos.c 中创建的互斥锁
extern SemaphoreHandle_t xLogMutex;

/**
 * @brief 初始化环形缓冲区
 */
void RingBuffer_Init(void) {
    g_LogBuffer.head = 0;
    g_LogBuffer.tail = 0;
    g_LogBuffer.count = 0;
}

/**
 * @brief 向环形缓冲区写入一条新数据 (覆盖老数据)
 */
void RingBuffer_Write(SensorData_t *newData) {
    // 获取互斥锁，最多阻塞等待 10ms，防止死锁
    if (xSemaphoreTake(xLogMutex, pdMS_TO_TICKS(10)) == pdTRUE) {

        // 写入最新数据到头部
        g_LogBuffer.buffer[g_LogBuffer.head] = *newData;

        // 头指针向前推进一步，遇顶折返 (环形覆盖机制)
        g_LogBuffer.head = (g_LogBuffer.head + 1) % LOG_MAX_RECORDS;

        // 如果数据满了，尾指针也要跟着动，意味着最老的数据被覆盖了
        if (g_LogBuffer.count < LOG_MAX_RECORDS) {
            g_LogBuffer.count++;
        } else {
            g_LogBuffer.tail = (g_LogBuffer.tail + 1) % LOG_MAX_RECORDS;
            //强行把 tail（读指针）也往前推一格！这意味着最老的那一条传感器记录被物理遗弃了，永远保持缓冲区里是最新的 LOG_MAX_RECORDS 条记录。
        }

        // 写入完毕，务必释放互斥锁
        xSemaphoreGive(xLogMutex);
    }
}
