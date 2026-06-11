#include "debug_log.h"
#include "usart.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// 🟢 引入我们在 freertos.c 里创建的那个锁，并包含 RTOS 头文件
#include "FreeRTOS.h"
#include "semphr.h"
extern SemaphoreHandle_t xLogMutex;

static uint8_t debug_log_enable = 1;

void Debug_Log_Init(void)
{
    debug_log_enable = 1;
}
//设计思路：
//这个函数是对 C 标准库 printf 的完美平替。它的核心工作流是：
//抢锁：排队等候，拿到串口使用权。
//组装：在自己的私有内存（栈空间）里，把 %d, %s 这些占位符，替换成真实的数字和文字。
//发送：调用 HAL 库，把组装好的文字一波推给底层硬件。
//还锁：让出串口，让下一个任务打印。
//参数 ...：这是 C 语言的“可变参数”语法。因为你不知道用户会传多少个参数进来（比如 Debug_Printf("T=%d", t) 是 2 个参数
//Debug_Printf("T=%d, H=%d", t, h) 是 3 个参数），用 ... 就可以照单全收。
void Debug_Printf(const char *fmt, ...)
{
    if (!debug_log_enable)// 如果总开关关了，直接闪退
        return;

    // 🟢 1. 获取锁！谁先抢到锁谁打印，其他任务乖乖排队等！
    if (xLogMutex != NULL) {
        xSemaphoreTake(xLogMutex, portMAX_DELAY);
    }

    char buf[128];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    if (len > 0)
    {
    	//尽管 vsnprintf 限制了实际写入 buf 的字符不超过 128 字节
    	//但 C 标准库规定，vsnprintf 的返回值 len 反映的是“如果缓冲区无限大，实际需要生成的字符串总长度”。
    	//外部传入一个超长格式化串，需要 150 字节，vsnprintf 实际往 buf 写了 127 字节外加一个 \0，但其返回值 len 仍然是 150。
    	//如果直接将 len（150）传给底层发送函数，硬件就会在读取完 buf（128字节）后，继续向后读取 22 字节的栈内存敏感数据并发送出去
    	//此处的判断强制将 len 截断为 127（即 sizeof(buf) - 1），绝对避免了越界读取。
        if (len >= sizeof(buf)) {
            len = sizeof(buf) - 1;
        }
        //将通过验证的纯净字符串数据交付给 STM32 的硬件串口 1 模块执行物理输出。
        //100 最大硬件超时时间（Timeout，单位：毫秒）。
        HAL_UART_Transmit(&huart1, (uint8_t *)buf, len, 100);
    }

    // 🟢 2. 打印完了，释放锁！让给下一个任务
    if (xLogMutex != NULL) {
        xSemaphoreGive(xLogMutex);
    }
}
