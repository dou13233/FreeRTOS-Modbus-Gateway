/*
 * dht11.c
 *
 *  Created on: 2026年5月29日
 *      Author:dd
 */


#include "dht11.h"
#include "gpio.h"   // 包含 HAL 的 GPIO 操作

/* ==================================================================== */
/* 1. 使用 Cortex-M3 内核 DWT 计数器实现的绝对精确微秒延时 */
/* ==================================================================== */
static void DHT11_Delay_us(uint32_t us)
{
    // 开启 Cortex-M3 DWT 周期计数器
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
   // DWT->CTRL |= DWT_CTRL_CYCCNT_Msk;
    DWT->CTRL |= 1;  // 🟢 直接写 1，代表激活 DWT 计数器，兼容所有库！

    // 获取当前硬件计数值
    uint32_t startTick = DWT->CYCCNT;

    // 72MHz 主频下，1微秒等于 72 个时钟周期
    uint32_t targetTicks = us * 72;

    // 精准死等，直到经过了目标周期数
    while ((DWT->CYCCNT - startTick) < targetTicks);
}

// 毫秒延时保持不变
static void DHT11_Delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        for (uint32_t j = 0; j < 8000; j++) {
            __NOP();
        }
    }
}

/* 发送起始信号（开漏模式下输出低电平，然后释放总线） */
static void DHT11_Start(void)
{
    // 开漏模式下，输出低电平
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    DHT11_Delay_ms(20);                                      // 拉低至少 18ms
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);  // 释放总线
    DHT11_Delay_us(30);                                      // 等待 20~40us
}

/* ==================================================================== */
/* 2. 优化后的单字节读取逻辑（边缘检测 + 绝对精准延时） */
/* ==================================================================== */
static uint8_t DHT11_Read_Byte(void)
{
    uint8_t byte = 0;
    uint32_t timeout;

    for (int i = 0; i < 8; i++)
    {
        timeout = 0;
        // 1. 等待变为低电平 (该数据位的开始)
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
            if (++timeout > 1000) return 0;
        }

        timeout = 0;
        // 2. 等待变为高电平 (跳过 50us 的低电平前导码)
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
            if (++timeout > 1000) return 0;
        }

        // 3. 此时电平刚刚变高！
        // 规则：'0' 的高电平持续 26~28us，'1' 的高电平持续 70us。
        // 所以我们极其精准地延时 40us：
        DHT11_Delay_us(40);

        // 4. 40us 后采样电平：如果还是高电平，说明是 '1'；如果是低，说明是 '0'。
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET)
        {
            byte |= (1 << (7 - i)); // 把该位置 1

            timeout = 0;
            // 5. 等待剩下的高电平结束，为下一位数据做准备
            while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
                if (++timeout > 1000) return byte;
            }
        }
    }
    return byte;
}
/* 主读取函数：成功返回 0，失败返回非 0 */
uint8_t DHT11_Read_Data(float *temperature, float *humidity)
{
    uint8_t buffer[5] = {0};

    // 1. 主机发送开始信号
    DHT11_Start();

    // 2. 等待 DHT11 响应（低电平 80us，高电平 80us）
    uint32_t timeout = 0;
    // 等待低电平（响应信号开始）
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET && timeout++ < 200)
        DHT11_Delay_us(1);
    if (timeout >= 200) return 1;   // 响应超时

    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET && timeout++ < 200)
        DHT11_Delay_us(1);
    timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET && timeout++ < 200)
        DHT11_Delay_us(1);

    // 3. 读取 40 位数据（5 个字节）
    for (int i = 0; i < 5; i++)
    {
        buffer[i] = DHT11_Read_Byte();
    }

    // 4. 校验和验证
    if (buffer[4] == (buffer[0] + buffer[1] + buffer[2] + buffer[3]))
    {
        *humidity = (float)buffer[0];
        *temperature = (float)buffer[2];
        return 0;   // 成功
    }
    else
    {
        return 2;   // 校验失败
    }
}
