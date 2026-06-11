/*
 * storage_service.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_STORAGE_SERVICE_H_
#define APPLICATION_USER_CORE_INC_STORAGE_SERVICE_H_

#include "main.h"

// 参数版本号 (防呆设计：以后如果增减参数，改一下版本号，系统就能自动格式化旧数据)
#define PARAM_VERSION_MAGIC 0xA002

// 系统全局参数池
typedef struct {
    uint16_t version;       // 版本验证码
    uint16_t modbus_addr;   // 本机 Modbus 地址 [cite: 64]

    // 我们把 float 放大 10 倍存成 uint16_t，避免浮点数在 Flash 里的对齐问题
    uint16_t temp_high_limit;  // 温度报警上限 (例: 305 代表 30.5度)
    uint16_t smoke_high_limit; // 烟雾报警上限 (例: 50 代表 0.5)
} AppParam_t;

// 对外暴露的全局参数和接口
extern AppParam_t g_AppParam;

void Storage_Init(void);
void Storage_Save(void);

#endif /* APPLICATION_USER_CORE_INC_STORAGE_SERVICE_H_ */
