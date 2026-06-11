/*
 * modbus_service.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_MODBUS_SERVICE_H_
#define APPLICATION_USER_CORE_INC_MODBUS_SERVICE_H_

#include "main.h"

typedef enum {//Modbus 通信状态机
    COMM_IDLE = 0,       // 空闲，等待新帧
    COMM_RECEIVING,      // DMA 正在接收数据
    COMM_PROCESSING,     // 收到完整帧，ModbusTask 正在解析
    COMM_ERROR           // CRC 错误或超时
} CommState_e;

// 声明全局状态机变量，供其他文件使用
extern CommState_e g_CommState;
// 定义本网关的 Modbus 设备地址（假设我们是 1 号设备）
//#define SLAVE_ADDRESS 0x01

// 声明外部调用的解析函数
void Modbus_Parse_Frame(uint8_t *buf, uint16_t len);

#endif /* APPLICATION_USER_CORE_INC_MODBUS_SERVICE_H_ */
/*
 * modbus_service.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_MODBUS_SERVICE_H_
#define APPLICATION_USER_CORE_INC_MODBUS_SERVICE_H_

#include "main.h"

// 🟢 新增：Modbus 通信状态机枚举 [cite: 21, 22, 23, 24]
typedef enum {
    COMM_IDLE = 0,       // 空闲，等待新帧
    COMM_RECEIVING,      // DMA 正在接收数据
    COMM_PROCESSING,     // 收到完整帧，ModbusTask 正在解析
    COMM_ERROR           // CRC 错误或超时
} CommState_e;

// 声明全局状态机变量，供其他文件使用
extern CommState_e g_CommState;
// 定义本网关的 Modbus 设备地址（假设我们是 1 号设备）
#define SLAVE_ADDRESS 0x01

// 声明外部调用的解析函数
void Modbus_Parse_Frame(uint8_t *buf, uint16_t len);

#endif /* APPLICATION_USER_CORE_INC_MODBUS_SERVICE_H_ */
