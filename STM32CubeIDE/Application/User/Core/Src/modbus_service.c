/*
 * modbus_service.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "modbus_service.h"
#include <stdio.h>
#include "drv_uart_dma.h"
#include "storage_service.h"
#include "debug_log.h"
extern AppParam_t g_AppParam; // 声明外部的全局参数结构体（名字根据你的实际定义调整）
/**
 * @brief 工业级 Modbus CRC16 校验算法
 * @param buf 要计算的数据首地址
 * @param len 要计算的长度
 * @return 16位的校验码
 */

typedef struct {
    float temperature;
    float humidity;
    float smoke;
    uint32_t timestamp;//开机时间戳
} SensorData_t;

extern SensorData_t g_LatestSensorData;
//CRC-16 循环冗余校验
//作用：防止 RS485 线缆在强电磁场下发生 bit 翻转导致数据错乱
static uint16_t Modbus_CRC16(uint8_t *buf, uint16_t len)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}
#if 0
/**
 * @brief Modbus 数据帧统一解析入口
 * @param buf 收到的完整数据帧数组
 * @param len 实际收到的长度
 */
void Modbus_Parse_Frame(uint8_t *buf, uint16_t len)
{
	// 🟢 1. 进来第一步，先打印到底收到了什么鬼东西
	    Debug_Printf("[Modbus Parse] 收到原始数据: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
	                 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

	    // 🟢 2. 检查第一道门：地址
	    if (buf[0] != g_AppParam.modbus_addr && buf[0] != 0x00)
	    {
	        Debug_Printf("[Modbus 拦截] 地址不对！本机:%02X, 收到:%02X\r\n", g_AppParam.modbus_addr, buf[0]);
	        return; // 原本的代码在这里默默返回了
	    }

	    // 🟢 3. 检查第二道门：你的代码里大概率有个算 CRC 的地方，在那个判断出错的地方加上打印：
	    // (下面这句需要根据你原本的代码逻辑稍微套用一下，把单片机自己算出来的 CRC 打印出来)
	    uint16_t my_crc = Modbus_CRC16(buf, len - 2);
	    Debug_Printf("[Modbus Parse] 本机算得的CRC应该是: %04X\r\n", my_crc);
    // 1. 最基础的长度过滤（最少也得有：地址1+功能码1+CRC2 = 4字节）
    if (len < 4) {
        printf("[Modbus Error] 长度太短: %d\r\n", len);
        return;
    }

    // 2. 地址过滤：如果收到的不是发给我的（0x01），或者不是广播地址（0x00），直接无视
    if (buf[0] != SLAVE_ADDRESS && buf[0] != 0x00) {
        printf("[Modbus Error] 地址不匹配: %02X\r\n", buf[0]);
        return;
    }

    // 3. 核心大招：CRC 校验验证
    // 计算前 len-2 个字节的 CRC（不把收到的那两个 CRC 算进去）
    uint16_t calc_crc = Modbus_CRC16(buf, len - 2);

    // 提取收到的 CRC（Modbus 规定 CRC 是低字节在前，高字节在后）
    uint16_t rx_crc = (buf[len - 1] << 8) | buf[len - 2];

    if (calc_crc != rx_crc) {
    	uint8_t cheat_msg[5] = {0xEE, 0xEE, 0xEE, calc_crc & 0xFF, (calc_crc >> 8) & 0xFF};

    	        drv_uart3_transmit(cheat_msg, 5); // 用底层的发送函数把正确答案发回去
        return;
    }

    printf("[Modbus Success] CRC校验通过！准备执行功能码...\r\n");

    // 4. 拆解功能码，执行对应操作（这里我们先搭个框架，等下步再填满）
    uint8_t function_code = buf[1];
    switch (function_code)
    {
        case 0x03: // 功能码 03：读取保持寄存器（比如读取温湿度数据）
        	printf("--> 上位机请求: 读取传感器数据 (03)\r\n");

        	            // 准备一个用于回复的数组
        	            uint8_t tx_buf[32];
        	            uint16_t tx_len = 0;

        	            uint16_t temp_val = (uint16_t)(g_LatestSensorData.temperature * 10);
        	            uint16_t humi_val = (uint16_t)(g_LatestSensorData.humidity * 10);
        	            uint16_t gas_val  = (uint16_t)(g_LatestSensorData.smoke * 100);

        	            // --- 开始组装 Modbus 响应报文 ---
        	            tx_buf[tx_len++] = SLAVE_ADDRESS; // [0] 设备地址 (0x01)
        	            tx_buf[tx_len++] = 0x03;          // [1] 功能码 (0x03)

        	            // 🟢 补丁1：上位机要 4 个寄存器，这里必须写 0x08 (8个字节)！
        	            tx_buf[tx_len++] = 0x08;          // [2] 数据字节数

        	            // [3][4] 第1个寄存器：温度数据
        	            tx_buf[tx_len++] = (temp_val >> 8) & 0xFF;
        	            tx_buf[tx_len++] = temp_val & 0xFF;

        	            // [5][6] 第2个寄存器：湿度数据
        	            tx_buf[tx_len++] = (humi_val >> 8) & 0xFF;
        	            tx_buf[tx_len++] = humi_val & 0xFF;

        	            // [7][8] 第3个寄存器：烟雾数据
        	            tx_buf[tx_len++] = (gas_val >> 8) & 0xFF;
        	            tx_buf[tx_len++] = gas_val & 0xFF;

        	            // 🟢 补丁2：凑齐第4个寄存器 (系统状态：0表示正常，这里先填0)
        	            tx_buf[tx_len++] = 0x00;
        	            tx_buf[tx_len++] = 0x00;

        	            // --- 计算我们要回复数据的 CRC 校验码 ---
        	            uint16_t tx_crc = Modbus_CRC16(tx_buf, tx_len);

        	            // 填入 CRC (低位在前，高位在后)
        	            tx_buf[tx_len++] = tx_crc & 0xFF;
        	            tx_buf[tx_len++] = (tx_crc >> 8) & 0xFF;

        	            // 调用底层驱动发送
        	            drv_uart3_transmit(tx_buf, tx_len);
        	            printf("--> 响应数据发送完毕! 共 %d 字节\r\n", tx_len);

        	            break;

        case 0x05: // 功能码 05：写单个线圈（比如控制继电器/风扇）
        	printf("--> 上位机请求: 远程控制设备 (05)\r\n");

        	            // Modbus 0x05 指令格式规定：
        	            // buf[2][3] 是要控制的设备地址（比如 00 00 代表 1号设备，00 01 代表 2号）
        	            // buf[4][5] 是动作：FF 00 代表打开，00 00 代表关闭

        	            uint16_t device_addr = (buf[2] << 8) | buf[3]; // 提取要控制的地址
        	            uint16_t action      = (buf[4] << 8) | buf[5]; // 提取要执行的动作

        	            if (device_addr == 0x0000) // 假设地址 0x0000 代表板子上的自带 LED (PC13)
        	            {
        	                if (action == 0xFF00) {
        	                    // 收到打开指令，点亮 LED（注意：PC13通常是低电平点亮）
        	                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
        	                    printf("    [执行] 已远程开启 LED 灯！\r\n");
        	                }
        	                else if (action == 0x0000) {
        	                    // 收到关闭指令，熄灭 LED
        	                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
        	                    printf("    [执行] 已远程关闭 LED 灯！\r\n");
        	                }
        	            }

        	            // --- Modbus 0x05 协议规定：执行完动作后，要把收到的指令【原封不动】地发回去作为应答 ---
        	            // 因为我们要原样返回，所以直接把收到的 buf 发回去就行，连 CRC 都不用重新算！
        	            drv_uart3_transmit(buf, len);

        	            printf("--> 控制响应已返回给上位机!\r\n");
        	            break;
        case 0x06: // 功能码 06：写单个寄存器（用于远程修改参数）
                    printf("--> 上位机请求: 远程修改参数 (06)\r\n");

                    // Modbus 0x06 指令格式规定：
                    // buf[2][3] 是要写入的寄存器地址
                    // buf[4][5] 是要写入的具体数值

                    uint16_t reg_addr = (buf[2] << 8) | buf[3];
                    uint16_t reg_val  = (buf[4] << 8) | buf[5];

                    // 🟢 我们在通信协议里“人为约定”：地址 0x0004 代表温度报警阈值
                    if (reg_addr == 0x0004)
                    {
                        // 更新内存中的全局参数
                        // (注意：实际数值放大了10倍，上位机发 350 就代表 35.0℃)
                        g_AppParam.temp_high_limit = reg_val;

                        // 🟢 核心动作：立刻调用你之前写的 Flash 存储函数，永久保存！
                       Storage_Save();

                        printf("    [执行] 已远程将温度报警阈值修改为: %.1f C！\r\n", reg_val / 10.0f);
                    }
                    // 以后如果有烟雾阈值，可以加 else if (reg_addr == 0x0005) ...

                    // --- Modbus 0x06 协议规定：修改成功后，必须把收到的指令【原封不动】发回去 ---
                    drv_uart3_transmit(buf, len);

                    printf("--> 修改响应已返回给上位机!\r\n");
                    break;

        default:
            printf("[Modbus Error] 不支持的功能码: %02X\r\n", function_code);
            break;
    }
}
#endif
//Modbus_Parse_Frame：网关主路由决策中心
//安检 1：看长度够不够，太短的直接扔掉。
//安检 2：看设备地址，不是找我的或者不是广播，直接无视。
//安检 3：重新计算整帧数据的 CRC，与对端发来的 CRC 密文进行比对，对不上的直接拦截。
//业务分流：通过标准的 switch(function_code) 分流处理 03（读数据）、05（控开关）、06（改参数）。
void Modbus_Parse_Frame(uint8_t *buf, uint16_t len)
{
    // 1. 打印收到的原始数据
    Debug_Printf("[Modbus Parse] 收到原始数据: %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
                 buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);

    // 2. 基础长度过滤
    if (len < 4) {//一个最合法的 Modbus 帧至少包含：地址(1B)+功能码(1B)+CRC(2B)=4字节
        Debug_Printf("[Modbus Error] 长度太短: %d\r\n", len);
        return;
    }

    // 3. 地址过滤
    //条件 1 (buf[0] != g_AppParam.modbus_addr)：看包裹上的收件人是不是自己。
    //条件 2 (buf[0] != 0x00)：0x00 在 Modbus 协议里叫“广播地址”。意思是电脑拿着大喇叭喊：“所有人立刻急停！”。哪怕你的地址是 1 号，听到 0 号广播，你也必须无条件执行。
    if (buf[0] != g_AppParam.modbus_addr && buf[0] != 0x00) {
        Debug_Printf("[Modbus 拦截] 地址不对！本机:%02X, 收到:%02X\r\n", g_AppParam.modbus_addr, buf[0]);
        return;
    }

    // 4. CRC 校验验证
    //Modbus 数据包的内容（比如地址、寄存器数据）全部是大端传输
    //但是唯独结尾的 CRC 校验码，必须是小端传输！
    uint16_t calc_crc = Modbus_CRC16(buf, len - 2);//调用前面的函数，计算“除去最后2个字节（CRC本身）”之前所有负载数据的正确 CRC
    Debug_Printf("[Modbus Parse] 本机算得的CRC应该是: %04X\r\n", calc_crc);

    uint16_t rx_crc = (buf[len - 1] << 8) | buf[len - 2];
    //Modbus 规定线缆上传输的 CRC 是低字节在前(buf[len-2])，高字节在后(buf[len-1])。
    // 这里通过左移 8 位并进行位或（|）拼接，还原出对端发过来的 16位 CRC 密文。
    if (calc_crc != rx_crc) {
        Debug_Printf("[Modbus Error] CRC校验失败！计算值:%04X, 收到值:%04X\r\n", calc_crc, rx_crc);
        return;
    }

    // 🟢 所有的 printf 都已经被替换为 Debug_Printf，不再吞日志！
    Debug_Printf("[Modbus Success] CRC校验通过！准备执行功能码: %02X\r\n", buf[1]);

    uint8_t function_code = buf[1];//提取功能码（第 1 字节）
    switch (function_code)
    {
        case 0x03: // 功能码 03：读取保持寄存器
        {
            uint16_t reg_addr = (buf[2] << 8) | buf[3];//提取主站想要读取的起始寄存器地址
            uint16_t reg_num  = (buf[4] << 8) | buf[5];//提取主站想要连续读取的寄存器个数
            Debug_Printf("--> 上位机请求: 读取寄存器 (03), 起始地址:%04X, 数量:%d\r\n", reg_addr, reg_num);

            uint8_t tx_buf[32];// 准备网关自己的发货背包
            uint16_t tx_len = 0;

            tx_buf[tx_len++] = buf[0];      // [0] 设备地址
            tx_buf[tx_len++] = 0x03;        // [1] 功能码
            tx_buf[tx_len++] = reg_num * 2; // [2] 数据字节数 (1个寄存器是2字节)

            // 🟢 聪明的路由逻辑：判断上位机到底想读什么？
            if (reg_addr == 0x0004)
            {
                // 如果请求的是 0004 地址，精确返回内存里存的报警阈值 拆成高低 8 位强行塞入发货包
                tx_buf[tx_len++] = (g_AppParam.temp_high_limit >> 8) & 0xFF;
                tx_buf[tx_len++] = g_AppParam.temp_high_limit & 0xFF;
            }
            else
            {
                // 如果请求别的地址，默认当作读取传感器数据处理 (做了简化)
            	//核心换算：因为 Modbus 寄存器只能存整数，所以把浮点数乘 10，变成固定 1 位小数的整型
            	//// 比如 28.5°C 乘 10 变成 285 (16进制为 0x011D)
                uint16_t temp_val = (uint16_t)(g_LatestSensorData.temperature * 10);
                uint16_t humi_val = (uint16_t)(g_LatestSensorData.humidity * 10);

                if (reg_num >= 1) { tx_buf[tx_len++] = (temp_val >> 8) & 0xFF;//// 填入温度高 8 位
                //& 0xFF 是强制要求
                tx_buf[tx_len++] = temp_val & 0xFF; //// 填入温度低 8 位
                }
                if (reg_num >= 2) { tx_buf[tx_len++] = (humi_val >> 8) & 0xFF; // 填入湿度高 8 位
                tx_buf[tx_len++] = humi_val & 0xFF; }// 填入湿度低 8 位
            }

            // 计算 CRC 并填入尾部
            uint16_t tx_crc = Modbus_CRC16(tx_buf, tx_len);
            tx_buf[tx_len++] = tx_crc & 0xFF;
            tx_buf[tx_len++] = (tx_crc >> 8) & 0xFF;

            drv_uart3_transmit(tx_buf, tx_len);
            Debug_Printf("--> 响应发送完毕! 共 %d 字节\r\n", tx_len);
            break;
        }

        case 0x05: // 功能码 05：写单个线圈
        {
            Debug_Printf("--> 上位机请求: 远程控制设备 (05)\r\n");
            uint16_t device_addr = (buf[2] << 8) | buf[3];// 提取要控制的继电器/引脚物理地址
            uint16_t action      = (buf[4] << 8) | buf[5];// 提取动作命令：FF00代表开，0000代表关
            //0x05 功能码（写单个线圈）强制规定，数据域必须是 FF 00 才代表 ON，00 00 才代表 OFF。发别的值均视为非法指令。
            if (device_addr == 0x0000) {//// 我们的人为约定：0x0000 地址指代网关板载状态灯
                if (action == 0xFF00) {
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                    Debug_Printf("    [执行] 已远程开启 LED 灯！\r\n");
                } else if (action == 0x0000) {
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                    Debug_Printf("    [执行] 已远程关闭 LED 灯！\r\n");
                }
            }

            drv_uart3_transmit(buf, len); // 原样返回
            //规定 一旦从站成功执行了动作，它必须将主站发过来的整包数据（包括地址、功能码、数据等）一个字节不差地回传给主站。主站收到这个一模一样的包，就认为是一次成功的 ACK（确认应答）。
            Debug_Printf("--> 控制响应已返回给上位机!\r\n");
            break;
        }

        case 0x06: // 功能码 06：写单个寄存器
        {
            Debug_Printf("--> 上位机请求: 远程修改参数 (06)\r\n");
            uint16_t reg_addr = (buf[2] << 8) | buf[3];// 提取目标配置项在网关内存里的物理寄存器映射地址
            uint16_t reg_val  = (buf[4] << 8) | buf[5];// 提取主站发来的全新设定值（如 350 代表 35.0°C）

            if (reg_addr == 0x0004) {// 约定 0x0004 为系统的温度高限报警卡槽
                g_AppParam.temp_high_limit = reg_val;
                Storage_Save();

                // 🟢 极其关键：把之前带 %.1f 的浮点打印改成了整数除法，彻底杜绝隐性死机！
                //使用了整数除法（/10）和取余（%10）来模拟浮点打印，彻底规避了 C 标准库 printf 打印浮点数时偶尔引发的堆栈溢出导致网关死机的隐患
                Debug_Printf("    [执行] 已远程将温度报警阈值修改为: %d.%d C！\r\n", reg_val / 10, reg_val % 10);
            }

            drv_uart3_transmit(buf, len); // 原样返回
            //规定 一旦从站成功执行了动作，它必须将主站发过来的整包数据（包括地址、功能码、数据等）一个字节不差地回传给主站。主站收到这个一模一样的包，就认为是一次成功的 ACK（确认应答）。
            Debug_Printf("--> 修改响应已返回给上位机!\r\n");
            break;
        }

        default:
            Debug_Printf("[Modbus Error] 不支持的功能码: %02X\r\n", function_code);
            break;
    }
}
