/*
 * storage_service.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "storage_service.h"
#include "drv_flash.h"
#include <stdio.h>
#define printf(...) (void)0
// 实例化全局参数池
AppParam_t g_AppParam;

/**
 * @brief 系统上电初始化：加载参数。若为第一次运行或版本号不对，则写入默认值 [cite: 65]
 */
void Storage_Init(void)
{
    // 算出结构体占用多少个 16位的半字 在 STM32F1 系列中，内部 Flash 的读写最小物理单位是半字
    uint16_t words_to_read = sizeof(AppParam_t) / 2;

    // 1. 从 Flash 读取数据到内存
    //uint32_t ReadAddr去这块具体的硬盘扇区去读数据
    //这个地址绝不能随便挑！STM32 的代码也是存在 Flash 里的（通常从 0x08000000 开始排）。如果你把存储参数的地址选在了代码区，一调用 Write 函数，
    //你就会把自己的代码当场抹除，单片机瞬间成砖。 所以这个地址通常选在 Flash 的最末尾一页。
    //(uint16_t*)&g_AppParam
    //words_to_read 搬运的数量 底层 Flash 硬件只认 16 位
    drv_Flash_Read(FLASH_STORAGE_ADDR, (uint16_t*)&g_AppParam, words_to_read);

    // 2. 核心防呆判断：检查版本号对不对？
    if (g_AppParam.version != PARAM_VERSION_MAGIC)
    {
        printf("[Storage] 首次运行或版本不匹配，正在恢复出厂默认值...\r\n");

        // 赋予默认值
        g_AppParam.version = PARAM_VERSION_MAGIC;
        g_AppParam.modbus_addr = 0x01;
        g_AppParam.temp_high_limit = 400; // 默认 40.0 度
        g_AppParam.smoke_high_limit = 1200; // 默认 1200

        // 立即保存到 Flash
        Storage_Save();
    }
    else
    {
        printf("[Storage] 参数加载成功！ModbusAddr:%d, TempLimit:%.1f\r\n",
               g_AppParam.modbus_addr, g_AppParam.temp_high_limit / 10.0f);
    }
}

/**
 * @brief 保存当前内存中的参数到 Flash
 */
void Storage_Save(void)
{
    uint16_t words_to_write = sizeof(AppParam_t) / 2;// 算出需要写多少个 16位的半字
    // 重要：在擦写 Flash 前，强行关闭单片机的所有中断！防止死机！
    __disable_irq();
    //写入flash
    drv_Flash_Write(FLASH_STORAGE_ADDR, (uint16_t*)&g_AppParam, words_to_write);
    // 🟢 写完之后，立刻恢复中断！
     __enable_irq();
    printf("[Storage] 参数已成功写入内部 Flash！\r\n");
}
