/*
 * drv_flash.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "drv_flash.h"
#include "stm32f1xx_hal.h"

/**
 * @brief 从 Flash 读取数据 (16位半字读取)
 */
//Flash 闪存的“先擦后写”物理定律。
//在 RAM（运行内存）里，你可以随便把 0 变成 1，也可以把 1 变成 0。但是，在 Flash 里，你只能把 1 变成 0！

void drv_Flash_Read(uint32_t ReadAddr, uint16_t *pBuffer, uint16_t NumToRead)
{
    uint16_t i;
    for(i = 0; i < NumToRead; i++) {
        // 直接通过指针读取物理地址内容
    	//2 * i：因为地址在内存里是按字节（8-bit）排的。但你要读的是 16-bit 的半字（占 2 个字节），所以每次地址必须往前跳 2 个格子。
    	//(__IO uint16_t*)（强制转换）：这是 STM32 官方库的宏定义，__IO 在底层展开其实是 C 语言的 volatile 关键字。
        pBuffer[i] = *(__IO uint16_t*)(ReadAddr + 2 * i);
    }
}

/**
 * @brief 擦除并写入数据到 Flash
 */
void drv_Flash_Write(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite)
{
    uint32_t PageError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;

    // 1. 解锁 Flash 控制寄存器
    HAL_FLASH_Unlock();

    // 2. 配置擦除参数：擦除 1 页
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;// 告诉控制器：我要按“页”来擦除
    EraseInitStruct.PageAddress = WriteAddr;// 告诉控制器：我要擦除包含这个地址的这一整页
    EraseInitStruct.NbPages = 1;// 告诉控制器：只炸毁 1 页就够了

    // 3. 执行擦除
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &PageError) != HAL_OK) {
        HAL_FLASH_Lock();
        return; // 擦除失败直接退出
    }

    // 4. 循环写入半字 (16-bit)
    //FLASH_TYPEPROGRAM_HALFWORD：极其关键！规定写入的格式是“半字（16位）”。如果你在这里选了 WORD（32位）但芯片不支持，硬件会直接死机。
    for (uint16_t i = 0; i < NumToWrite; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, WriteAddr + 2 * i, pBuffer[i]);
    }

    // 5. 重新上锁保护
    HAL_FLASH_Lock();
}
