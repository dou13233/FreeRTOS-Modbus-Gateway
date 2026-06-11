/*
 * dht11.h
 *
 *  Created on: 2026年5月29日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_DHT11_H_
#define APPLICATION_USER_CORE_DHT11_H_

#include "main.h"
#include <stdint.h>

// 修改为你实际使用的引脚（CubeMX 中 User Label 为 DHT11_DATA 的话）
#define DHT11_PORT        GPIOA
#define DHT11_PIN         GPIO_PIN_0

// 函数声明
uint8_t DHT11_Read_Data(float *temperature, float *humidity);


#endif /* APPLICATION_USER_CORE_DHT11_H_ */
