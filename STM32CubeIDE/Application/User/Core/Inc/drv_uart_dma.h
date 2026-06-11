/*
 * drv_uart_dma.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_DRV_UART_DMA_H_
#define APPLICATION_USER_CORE_INC_DRV_UART_DMA_H_

#include "main.h"

// 定义 Modbus 接收缓冲区的最大长度（一帧数据通常不会超过 128 字节）
#define MODBUS_RX_BUF_SIZE 128

// 声明外部变量，让其他文件（比如中断文件和 Modbus 解析任务）能用到它们
extern uint8_t Modbus_RxBuf[MODBUS_RX_BUF_SIZE];
extern uint16_t Modbus_RxLen;

// 初始化函数声明
void drv_uart3_dma_init(void);
// 发送数据的函数声明
void drv_uart3_transmit(uint8_t *data, uint16_t len);
#define UART_RX_RING_SIZE 256 // 环形缓冲区大小，256字节足够容纳多个 Modbus 帧

// 从环形缓冲区中提取一帧数据
uint16_t drv_uart_read_ringbuffer(uint8_t *dest, uint16_t max_len);

// 供串口 IDLE 中断调用的压入函数
void drv_uart_push_to_ringbuffer(uint8_t *data, uint16_t len);
#endif /* APPLICATION_USER_CORE_INC_DRV_UART_DMA_H_ */
