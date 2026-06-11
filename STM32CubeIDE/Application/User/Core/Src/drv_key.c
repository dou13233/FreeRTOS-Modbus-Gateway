/*
 * drv_key.c
 *
 *  Created on: 2026年6月1日
 *      Author:dd
 */

#include "drv_key.h"
#include "gpio.h"

static Key_t g_key1, g_key2, g_key3;

/* 读取管脚电平 (根据 CubeMX 实际引脚配置对齐) */
// KEY1 -> PB0
static uint8_t Key1_ReadPin(void) { return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) ? 1 : 0; }
// KEY2 -> PB1
static uint8_t Key2_ReadPin(void) { return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_1) == GPIO_PIN_RESET) ? 1 : 0; }
// KEY3 -> PA4 (根据你的截图修正)
static uint8_t Key3_ReadPin(void) { return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET) ? 1 : 0; }

void drv_Key_Init(void)
{
    g_key1.state = KEY_STATE_IDLE; g_key1.readPin = Key1_ReadPin; g_key1.event = KEY_EVENT_NONE;
    g_key2.state = KEY_STATE_IDLE; g_key2.readPin = Key2_ReadPin; g_key2.event = KEY_EVENT_NONE;
    g_key3.state = KEY_STATE_IDLE; g_key3.readPin = Key3_ReadPin; g_key3.event = KEY_EVENT_NONE;
}

/**
 * @brief 极简非阻塞状态机：只处理短按，按住不放不会连续触发
 */
static void Key_StateMachine(Key_t *key)
{
    uint8_t keyPressed = key->readPin();
    TickType_t currentTick = xTaskGetTickCount();

    switch (key->state)
    {
        case KEY_STATE_IDLE:
            if (keyPressed) {
                key->state = KEY_STATE_DEBOUNCE;
                key->pressStartTick = currentTick;
            }
            break;

        case KEY_STATE_DEBOUNCE:
            if ((currentTick - key->pressStartTick) >= pdMS_TO_TICKS(KEY_DEBOUNCE_TIME_MS)) {
                if (keyPressed) {
                    key->event = KEY_EVENT_PRESS;      // 确认是真实按下，丢出事件
                    key->state = KEY_STATE_WAIT_RELEASE; // 进入死等松手状态
                } else {
                    key->state = KEY_STATE_IDLE;       // 只是抖动，退回空闲
                }
            }
            break;

        case KEY_STATE_WAIT_RELEASE:
            if (!keyPressed) {
                key->state = KEY_STATE_IDLE;           // 松手了，重新准备迎接下一次按下
            }
            break;
    }
}

void drv_Key_Scan(void)
{
    Key_StateMachine(&g_key1);
    Key_StateMachine(&g_key2);
    Key_StateMachine(&g_key3);
}

KeyEvent_t Key1_GetEvent(void) { return g_key1.event; }
KeyEvent_t Key2_GetEvent(void) { return g_key2.event; }
KeyEvent_t Key3_GetEvent(void) { return g_key3.event; }

void Key1_ClearEvent(void) { g_key1.event = KEY_EVENT_NONE; }
void Key2_ClearEvent(void) { g_key2.event = KEY_EVENT_NONE; }
void Key3_ClearEvent(void) { g_key3.event = KEY_EVENT_NONE; }
