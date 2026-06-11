/*
 * drv_uart_dma.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "drv_uart_dma.h"
#include "usart.h" // 引入 huart3 句柄
#include "gpio.h"
#include "modbus_service.h"
#include "cmsis_os.h"
#include <stdio.h>
// 1. 定义真正的接收数据仓库
// 加上 __attribute__((aligned(4))) 是为了让内存在 4 字节边界对齐，这能让 DMA 搬运时最高效且不容易出错
__attribute__((aligned(4))) uint8_t Modbus_RxBuf[MODBUS_RX_BUF_SIZE];

// 2. 记录实际收到了多少个字节的数据
uint16_t Modbus_RxLen = 0;

/**
 * @brief  USART3 (RS485) DMA 接收与空闲中断初始化
 * @note   这个函数必须在 main() 里面的外设初始化完成之后、RTOS 调度器启动之前调用！
 */
void drv_uart3_dma_init(void)
{
    // 1. 开启 USART3 的 IDLE（空闲）中断
    // 硬件一旦检测到总线沉默，就会触发中断
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

    // 2. 启动 DMA 接收
    // 告诉 DMA：去盯着 huart3，有数据就全部搬进 Modbus_RxBuf，最多搬 MODBUS_RX_BUF_SIZE 个！
    HAL_UART_Receive_DMA(&huart3, Modbus_RxBuf, MODBUS_RX_BUF_SIZE);
}
/**
 * @brief RS485 发送数据函数 (带方向自动切换)
 * @param data 要发送的数据首地址
 * @param len 发送长度
 */

void drv_uart3_transmit(uint8_t *data, uint16_t len)
{
    //  1：超级避让延时！
    // 强制等 100 毫秒！让电脑端的 USB 模块有绝对充足的时间切换回“倾听”状态！
	//PA2（RS485_CTRL） PA2拉高 发送 PA2拉低 接收
    osDelay(100);

    //  2：打碎 HAL 库的枷锁
    // 强行清除底层可能因为刚接收完数据残留的错误标志，并强制解锁状态机
    __HAL_UART_CLEAR_OREFLAG(&huart3);
    huart3.gState = HAL_UART_STATE_READY;

    // 1. 将 PA2 (RS485_CTRL) 拉高，让单片机准备发送
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_SET);

    // 2. 阻塞发送数据，并用 status 变量把底层的真实情况抓出来！
    HAL_StatusTypeDef status = HAL_UART_Transmit(&huart3, data, len, 100);
    //阻塞发送函数 &huart3：用串口3发送 data：你要发送的包裹的首地址 len：你要发送多少个字节
    //100：超时时间（毫秒）。意思是我最多花 100ms 来发这些数据，如果 100ms 发不完就算了，系统不能一直卡死在这里。
    //status 是函数执行完的返回值，用来记录刚刚发送到底是成功了（HAL_OK），还是出错了。

    // 3. 发完后再等 5 毫秒确保线上数据飞完
    osDelay(5);

    // 4. 数据彻底发完后，立刻将 PA2 拉低，恢复为监听
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_2, GPIO_PIN_RESET);

    // =======================================================
    // 测谎仪打印：通过你的日志口(COM5)看看底层到底有没有发出去！
    if (status == HAL_OK) {
        printf("[底层真话] HAL_OK! %d 个字节已强行灌入硬件引脚！\r\n", len);
    } else {
        printf("[底层真话] 失败！HAL库拦截了发送，错误码: %d (1:错误, 2:忙碌, 3:超时)\r\n", status);
    }
    // =======================================================
}
// 实例化状态机变量，上电默认空闲
CommState_e g_CommState = COMM_IDLE;//Modbus 通信状态机

// 串口接收专用的环形缓冲区
typedef struct {
    uint8_t buffer[UART_RX_RING_SIZE];//256
    uint16_t head; // 写指针
    uint16_t tail; // 读指针
} UartRingBuffer_t;

static UartRingBuffer_t rx_ring;//static

/**
 * @brief 中断里做的事：将 DMA 收到的不定长数据暴力推入环形缓冲区
 * @note  这个函数必须由你的串口 IDLE 空闲中断来调用！
 */
void drv_uart_push_to_ringbuffer(uint8_t *data, uint16_t len)
{
    g_CommState = COMM_RECEIVING; // 状态机切入：接收中

    for (uint16_t i = 0; i < len; i++) {
        rx_ring.buffer[rx_ring.head] = data[i];
        rx_ring.head = (rx_ring.head + 1) % UART_RX_RING_SIZE;//取余 环形缓冲区 head是写指针
    }
}

/**
 * @brief 任务里做的事：从环形缓冲区把数据全部捞出来
 */
uint16_t drv_uart_read_ringbuffer(uint8_t *dest, uint16_t max_len)
{
    uint16_t len = 0;
    // 当读指针没追上写指针时，说明有数据 tail != head
    while (rx_ring.tail != rx_ring.head && len < max_len) {
        dest[len++] = rx_ring.buffer[rx_ring.tail];
        rx_ring.tail = (rx_ring.tail + 1) % UART_RX_RING_SIZE;//环形缓冲区 tail是读指针
    }
    return len;
}
