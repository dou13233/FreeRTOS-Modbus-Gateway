/*
 * drv_flash.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_DRV_FLASH_H_
#define APPLICATION_USER_CORE_INC_DRV_FLASH_H_

#include "main.h"

// STM32F103C8T6 共有 64KB Flash，每页 1KB
// 我们选用最后一页 (Page 63) 的起始地址，绝对安全，不会和代码区冲突
#define FLASH_STORAGE_ADDR 0x0800FC00

void drv_Flash_Read(uint32_t ReadAddr, uint16_t *pBuffer, uint16_t NumToRead);
void drv_Flash_Write(uint32_t WriteAddr, uint16_t *pBuffer, uint16_t NumToWrite);

#endif /* APPLICATION_USER_CORE_INC_DRV_FLASH_H_ */
