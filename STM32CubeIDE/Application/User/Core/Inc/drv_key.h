/*
 * drv_key.h
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#ifndef APPLICATION_USER_CORE_INC_DRV_KEY_H_
#define APPLICATION_USER_CORE_INC_DRV_KEY_H_

#include "main.h"
#include "FreeRTOS.h"
#include "task.h"

// 按键消抖时间 (单位: ms)
#define KEY_DEBOUNCE_TIME_MS 20

// 极简状态机枚举
typedef enum {
    KEY_STATE_IDLE = 0,     // 空闲
    KEY_STATE_DEBOUNCE,     // 消抖中
    KEY_STATE_WAIT_RELEASE  // 等待松手 (防连发)
} KeyState_t;

// 极简事件枚举 (只需要知道按下了就行)
typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_PRESS         // 按键被按下触发
} KeyEvent_t;

// 按键核心结构体
typedef struct {
    KeyState_t state;
    TickType_t pressStartTick;
    uint8_t (*readPin)(void);
    KeyEvent_t event;
} Key_t;

// 接口声明
void drv_Key_Init(void);
void drv_Key_Scan(void);

KeyEvent_t Key1_GetEvent(void);
KeyEvent_t Key2_GetEvent(void);
KeyEvent_t Key3_GetEvent(void);

void Key1_ClearEvent(void);
void Key2_ClearEvent(void);
void Key3_ClearEvent(void);

#endif /* APPLICATION_USER_CORE_INC_DRV_KEY_H_ */
