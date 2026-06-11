/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gpio.h"
#include <stdio.h>
#include <string.h>
#include "usart.h"
#include "dht11.h"
#include "mq2.h"
#include <stdio.h>
#include"ssd1306.h"
#include "drv_uart_dma.h"
#include "modbus_service.h"
#include "drv_key.h"
#include "ringbuffer.h"
#include "storage_service.h"
#include "cmd_parser.h"
#include "filter.h"
#include "debug_log.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
osThreadId ledTaskHandle;
// 传感器数据结构体

// 【Modbus DMA 接收信号量】
// 作用：这是一个二值信号量，相当于一把“起跑枪”。
// 初始状态：为空（0）。
// 触发机制：当串口3发生 IDLE 空闲中断时，中断服务函数会释放（Give）这个信号量。
// 响应机制：ModbusTask 一直在阻塞等待（Take）这个信号量，一旦拿到，立刻醒来处理数据。
SemaphoreHandle_t xModbusRxSemaphore = NULL;

// 🟢 引入我们在 drv_uart_dma.c 里定义的接收仓库和长度
extern uint8_t Modbus_RxBuf[128];
extern uint16_t Modbus_RxLen;
SensorData_t g_LatestSensorData = {0.0, 0.0, 0.0, 0};
typedef enum {
    KEY_NONE = 0,
    KEY_PAGE_SWITCH, // 按键1：页面切换
    KEY_PLUS,        // 按键2：数值加/上移
    KEY_MINUS        // 按键3：数值减/下移
} KeyEvent_e;

// 🟢 新增：保护日志读写的互斥锁
SemaphoreHandle_t xLogMutex = NULL;


// 🟢 补上缺失的：按键消息队列句柄
QueueHandle_t xKeyQueue;
// 队列句柄
QueueHandle_t xSensorQueue;

// 事件组句柄
EventGroupHandle_t xAlarmEventGroup;

// 事件位定义
//// 定义两盏专属的“报警灯”（在 24 个可用位中，我们挑了第0位和第1位）
#define BIT_TEMP_HIGH   (1 << 0)
#define BIT_SMOKE_HIGH  (1 << 1)
/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId SensorTaskHandle;
osThreadId ModbusTaskHandle;
osThreadId AlarmTaskHandle;
osThreadId DisplayTaskHandle;
osThreadId KeyScanTaskHandle;
osThreadId LogTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void StartLEDTask(void const * argument);
void StartUARTTask(void const * argument);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void StartSensorTask(void const * argument);
void StartModbusTask(void const * argument);
void StartAlarmTask(void const * argument);
void StartDisplayTask(void const * argument);
void StartKeyScanTask(void const * argument);
void StartLogTask(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
	HAL_Delay(500);
	Storage_Init();
	// 🟢 系统上电第一件事：强行把蜂鸣器引脚拉高，保证初始状态是安静的！
	  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
	xLogMutex = xSemaphoreCreateMutex();
	  if (xLogMutex == NULL) {
	      for(;;); // 如果内存不足创建失败，直接卡死报错
	  }
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
	xModbusRxSemaphore = xSemaphoreCreateBinary();
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
	// 创建队列，最多存放5个传感器数据
	xSensorQueue = xQueueCreate(5, sizeof(SensorData_t));
	if (xSensorQueue == NULL) {
	    // 处理错误，例如死循环
	    for(;;);
	}
	xKeyQueue = xQueueCreate(5, sizeof(KeyEvent_e));
	// 创建事件组
	xAlarmEventGroup = xEventGroupCreate();
	if (xAlarmEventGroup == NULL) {
	    for(;;);
	}



  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 128);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of SensorTask */
  osThreadDef(SensorTask, StartSensorTask, osPriorityBelowNormal, 0, 512);
  SensorTaskHandle = osThreadCreate(osThread(SensorTask), NULL);

  /* definition and creation of ModbusTask */
  osThreadDef(ModbusTask, StartModbusTask, osPriorityAboveNormal, 0, 384);
  ModbusTaskHandle = osThreadCreate(osThread(ModbusTask), NULL);

  /* definition and creation of AlarmTask */
  osThreadDef(AlarmTask, StartAlarmTask, osPriorityHigh, 0, 256);
  AlarmTaskHandle = osThreadCreate(osThread(AlarmTask), NULL);

  /* definition and creation of DisplayTask */
  osThreadDef(DisplayTask, StartDisplayTask, osPriorityLow, 0, 512);
  DisplayTaskHandle = osThreadCreate(osThread(DisplayTask), NULL);

  /* definition and creation of KeyScanTask */
  osThreadDef(KeyScanTask, StartKeyScanTask, osPriorityLow, 0, 256);
  KeyScanTaskHandle = osThreadCreate(osThread(KeyScanTask), NULL);

  /* definition and creation of LogTask */
  osThreadDef(LogTask, StartLogTask, osPriorityLow, 0, 256);
  LogTaskHandle = osThreadCreate(osThread(LogTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  //osThreadDef(LEDTask, StartLEDTask, osPriorityNormal, 0, 128);
   //ledTaskHandle = osThreadCreate(osThread(LEDTask), NULL);
    // osThreadDef(UARTTask, StartUARTTask, osPriorityLow, 0, 512);
     //osThreadCreate(osThread(UARTTask), NULL);
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartSensorTask */
/**
* @brief Function implementing the SensorTask thread.
* @param argument: Not used
* @retval None
*/

/* USER CODE END Header_StartSensorTask */
void StartSensorTask(void const * argument)
{
  /* USER CODE BEGIN StartSensorTask */
  float temp, humi, smoke;//原始数据
  SensorData_t data;

  // 🟢 给 DHT11 分配中值滤波器
  MedianFilter_t temp_filter, humi_filter;
  MedianFilter_Init(&temp_filter);
  MedianFilter_Init(&humi_filter);

  // 🟢 给 MQ-2 烟雾分配一阶低通滤波器 (取 0.3 的滤波系数，兼顾平滑与响应)
  EmaFilter_t smoke_filter;
  EmaFilter_Init(&smoke_filter, 0.3f);

  for(;;)
  {
      uint8_t ret = DHT11_Read_Data(&temp, &humi);
      smoke = MQ2_GetGasConcentration();

      if (ret == 0) {
          // 经过工业级滤波洗礼
          data.temperature = MedianFilter_Process(&temp_filter, temp);
          data.humidity = MedianFilter_Process(&humi_filter, humi);
          data.smoke = EmaFilter_Process(&smoke_filter, smoke);
      } else {
          osDelay(500);
          continue;
      }
      data.timestamp = xTaskGetTickCount();
      //将装满温湿度和烟雾数据的结构体，塞进信箱
      xQueueSend(xSensorQueue, &data, 0);

      // 工业常规采样率：500ms
      osDelay(500);
  }
  /* USER CODE END StartSensorTask */
}

/* USER CODE BEGIN Header_StartModbusTask */
/**
* @brief Function implementing the ModbusTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartModbusTask */
void StartModbusTask(void const * argument)
{
  /* USER CODE BEGIN StartModbusTask */
  Debug_Printf("ModbusTask Started! Architecture Upgraded.\r\n");

  // 任务内部准备一个局部数组，用来存放从传送带捞下来的数据
  //任务一醒来，第一件事就是用最快的速度把数据拷贝到自己私有的局部数组里，然后再慢慢做复杂的协议解析。这叫“数据解耦”。
  uint8_t local_frame_buf[128];
  uint16_t local_len = 0;

  for(;;)
  {
      // 1. 状态机：空闲等待中
      g_CommState = COMM_IDLE;

      // 2. 阻塞等待 DMA+IDLE 中断发来的信号量
      //这行代码会一直休眠。是谁把它叫醒的？是单片机的串口空闲中断（IDLE Interrupt）。当外部电脑发完一帧 Modbus 数据，串口总线上安静了超过 1 个字节的时间，单片机硬件就会触发空闲中断。
      //在中断服务函数（ISR）里，会调用 xSemaphoreGiveFromISR 释放信号量，瞬间唤醒这个任务。
      if (xSemaphoreTake(xModbusRxSemaphore, portMAX_DELAY) == pdTRUE)
      {
          // 3. 状态机：进入处理状态
          g_CommState = COMM_PROCESSING;

          // 4. 从环形缓冲区捞出刚才 DMA 塞进去的所有数据
          local_len = drv_uart_read_ringbuffer(local_frame_buf, sizeof(local_frame_buf));

          if (local_len > 0)
          {
              Debug_Printf("\r\n[ModbusTask] 从 RingBuffer 提取到 %d 字节.\r\n", local_len);

              // 5. 移交解析核心（如果解析发现 CRC 错误，可以在解析函数里把状态机切成 COMM_ERROR [cite: 24]）
              Modbus_Parse_Frame(local_frame_buf, local_len);
          }
      }
  }
  /* USER CODE END StartModbusTask */
}

/* USER CODE BEGIN Header_StartAlarmTask */
/**
* @brief Function implementing the AlarmTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartAlarmTask */
void StartAlarmTask(void const * argument)
{
  /* USER CODE BEGIN StartAlarmTask */

  // 1. 任务启动前，强制清空所有意外产生的报警标志，确保“干干净净”地开局
	//把第 0 位和第 1 位强制写为 0
  xEventGroupClearBits(xAlarmEventGroup, BIT_TEMP_HIGH | BIT_SMOKE_HIGH);
//uxBits：定义一个专门存事件状态的变量，用来接收系统唤醒你时，到底是哪几盏灯亮了。
  EventBits_t uxBits;
  //xTicksToWait：定义一个常量，portMAX_DELAY 在 FreeRTOS 里代表“无限期死等”，也就是不等到灯亮，这个任务永远不占用 CPU。
  const TickType_t xTicksToWait = portMAX_DELAY;

  for(;;)
  {
      // 2. 阻塞等待温度或烟雾超标事件
	  //函数作用：让当前任务进入睡眠状态，直到指定的事件位被置 1 才醒来。
	  //参数 1 xAlarmEventGroup：你要盯着的事件组。
	  //参数 2 BIT_TEMP_HIGH | BIT_SMOKE_HIGH (0000 0011)：你具体要盯着哪几个位。
	  //参数 3 pdTRUE (退出时是否清零)：设置为 TRUE。意味着只要灯亮了把你叫醒，系统会自动帮你把灯关掉（清 0）。这样你就不用手动写代码去清除了。
	  //参数 4 pdFALSE (触发逻辑，极度重要)：
	  //如果是 pdTRUE：表示逻辑与(AND)，必须温度和烟雾同时超标，才会唤醒。
	  //我们设为 pdFALSE：表示逻辑或(OR)，只要其中任何一个超标，立刻唤醒。
	  //参数 5 xTicksToWait：上面定义的死等参数。
      uxBits = xEventGroupWaitBits(
                  xAlarmEventGroup,
                  BIT_TEMP_HIGH | BIT_SMOKE_HIGH,
                  pdTRUE,
                  pdFALSE,
                  xTicksToWait );

      // 3. 只要收到事件，执行最终的“双重核实”
      if( ( uxBits & (BIT_TEMP_HIGH | BIT_SMOKE_HIGH) ) != 0 )
      {
          // 实时计算当前系统要求的阈值
          float temp_threshold = g_AppParam.temp_high_limit / 10.0f;
          float smoke_threshold = g_AppParam.smoke_high_limit;

          // 再次核实当前传感器数据确实超标，防止被其他意外置位误导
          if (g_LatestSensorData.temperature > temp_threshold ||
              g_LatestSensorData.smoke > smoke_threshold)
          {
        	  Debug_Printf("🚨 触发报警！当前温度: %d.%d, 烟雾: %d\r\n",
        	                       (int)g_LatestSensorData.temperature,
        	                       (int)(g_LatestSensorData.temperature * 10) % 10,
        	                       (int)g_LatestSensorData.smoke);

              // 滴-滴-滴 声光报警控制逻辑 (低电平触发)
              for(int i = 0; i < 3; i++)
              {
                  // 响！(拉低引脚)
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
                  osDelay(200);

                  // 停！(拉高引脚)
                  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
                  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
                  osDelay(200);
              }

              // 报警周期结束后强制冷静 2 秒，避免系统被蜂鸣器中断死锁
              osDelay(2000);
          }
      }
  }
  /* USER CODE END StartAlarmTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void const * argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  SensorData_t data;
  KeyEvent_e keyEvent;

  uint8_t current_page = 0; // 0:主界面, 1:详情页, 2:配置页
  osDelay(1500);
  SSD1306_Init();
  SSD1306_Clear();

  for(;;)
  {
      // 1. 非阻塞读取按键队列 (换页与加减)
	  //这是 FreeRTOS 专门用来从队列中提取数据的 API。它不仅会把数据读出来，还会把读过的数据从队列中彻底删掉
	  //xKeyQueue (目标信箱)
	  //对应发送端： 邮递员往哪个信箱塞信。
	  //在这里的作用： 告诉操作系统，屏幕刷新任务要去哪一个信箱里拿信。必须要和按键任务塞的是同一个 xKeyQueue，否则就拿错信了。
	 // &keyEvent (拆信的托盘/接收变量)
	  //对应发送端： &queueMsg 是信的原文地址（系统把它复印进信箱）。
	  //在这里的作用： 这是一个定义在屏幕任务里的局部变量的内存地址。当操作系统发现信箱里有信时，它会把信箱里存放的枚举值（比如 KEY_PAGE_SWITCH），原封不动地复制到 keyEvent 这个变量里。
	  //结果： 执行完这句代码后，你的 keyEvent 变量里就真正装上了用户刚才按下的按键代号，你就可以拿着它去写 if-else 来切页面了。
	  //0 (阻塞超时时间)
	  //对应发送端： 发送端的 0 意思是“信箱满了我不等，信扔了直接走”。
	  //在这里的作用： 这里的 0 意思是“信箱空了我绝对不傻等，立刻回去干别的工作”。这就是所谓的非阻塞读取。
	  //返回值判定： * 如果拿到信了，函数返回 pdTRUE，进入 if 里面去切换页面。
	  //如果信箱是空的（没人按按键），函数立刻返回 pdFALSE，直接跳过按键处理逻辑。
      if (xQueueReceive(xKeyQueue, &keyEvent, 0) == pdTRUE)
      {
          if (keyEvent == KEY_PAGE_SWITCH) {
              // 【核心联动】：如果当前是配置页，按下切换键离开时，执行保存逻辑！
              if (current_page == 2) {
                  Storage_Save(); // 写入 Flash！
            	 // Debug_Printf("[TEST] Skip Storage_Save()\r\n");
              }
              current_page = (current_page + 1) % 3;
              SSD1306_Clear();
          }
          else if (keyEvent == KEY_PLUS && current_page == 2) {
              g_AppParam.temp_high_limit += 10; // 加 1.0 度
              SSD1306_Clear();
          }
          else if (keyEvent == KEY_MINUS && current_page == 2) {
              g_AppParam.temp_high_limit -= 10; // 减 1.0 度
              SSD1306_Clear();
          }
      }

      // 2. 非阻塞读取传感器队列 (更新最新数据并判断报警)
      if (xQueueReceive(xSensorQueue, &data, 0) == pdTRUE) {
          g_LatestSensorData = data;

          // ================= [防抖判定 1：温度使用"死区回差法" + 电平触发] =================
          static uint8_t is_temp_alarming = 0; // 记录当前状态
          float temp_threshold = g_AppParam.temp_high_limit / 10.0f;
          float deadband = 0.5f; // 0.5度的死区

          if (!is_temp_alarming) {
              // 没报警时，必须越过 (阈值 + 死区) 才进入报警状态
              if (data.temperature > (temp_threshold + deadband)) {
                  is_temp_alarming = 1;
              }
          } else {
              // 报警中时，必须降到 (阈值 - 死区) 以下才安全解除
              if (data.temperature < (temp_threshold - deadband)) {
                  is_temp_alarming = 0;
                  xEventGroupClearBits(xAlarmEventGroup, BIT_TEMP_HIGH); // 安全解除
              }
          }

          // 🟢 核心改动 1：只要处于报警状态，持续推送事件位！(不听话就一直叫)
          if (is_temp_alarming) {
              xEventGroupSetBits(xAlarmEventGroup, BIT_TEMP_HIGH);
          }

          // ================= [防抖判定 2：烟雾使用"时间确认窗法" + 电平触发] =================
          static uint8_t smoke_over_count = 0; // 超标计数器
          float smoke_threshold = g_AppParam.smoke_high_limit;

          if (data.smoke > smoke_threshold) {
              smoke_over_count++;
              // 连续 4 次 (4 * 500ms = 2秒) 持续超标，才确认是真的火灾！
              if (smoke_over_count >= 4) {
                  smoke_over_count = 4; // 防止溢出
                  // 🟢 核心改动 2：只要确认为火灾，持续推送事件位！(电平触发)
                  xEventGroupSetBits(xAlarmEventGroup, BIT_SMOKE_HIGH);
              }
          } else {
              // 只要中间有一次低于阈值，立刻清零计数器，彻底解除警报！
              smoke_over_count = 0;
              xEventGroupClearBits(xAlarmEventGroup, BIT_SMOKE_HIGH);
          }
      }

      // 3. UI 渲染
     // SSD1306_Clear();
      SSD1306_GotoXY(0, 0);
      switch(current_page)
      {
          case 0:
              SSD1306_Puts("- MAIN -\n");
              SSD1306_Puts("T:");
              SSD1306_PrintFloat(g_LatestSensorData.temperature);
              SSD1306_Puts("C\n");
              SSD1306_Puts("H:");
              SSD1306_PrintFloat(g_LatestSensorData.humidity);
              SSD1306_Puts("%\n");
              break;
          case 1:
              SSD1306_Puts("- DETAILS -\n");
              SSD1306_Puts("Gas:"); SSD1306_PrintFloat(g_LatestSensorData.smoke);

              // 动态读取当前的报警标志位
              if (xEventGroupGetBits(xAlarmEventGroup) != 0) {
                  SSD1306_Puts("\nSts: ALARM!\n"); // 有任何超标，显示 ALARM
              } else {
                  SSD1306_Puts("\nSts: OK\n");     // 一切正常，显示 OK
              }
              break;
          case 2:
              SSD1306_Puts("- CONFIG -\n");
              SSD1306_Puts("T_ALARM:\n> ");
              SSD1306_PrintFloat(g_AppParam.temp_high_limit / 10.0f); // 显示真实参数
              SSD1306_Puts(" C\n");
              break;
      }
      SSD1306_Update();

      // 配合屏幕刷新率，这里延时200ms
      osDelay(200);
  }
  /* USER CODE END StartDisplayTask */
}

/* USER CODE BEGIN Header_StartKeyScanTask */
/**
* @brief Function implementing the KeyScanTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartKeyScanTask */
//设计思路：如果不把按键独立成任务，而是裸写在主循环或利用外部中断，极易因为按键抖动导致 CPU 频繁误触发，或者在长按时卡死其他逻辑。
//该任务以 20ms 为周期轮询，配合状态机消抖，既保证了按键的丝滑响应，又做到了完全不占用多余的 CPU。
void StartKeyScanTask(void const * argument)
{
  /* USER CODE BEGIN StartKeyScanTask */
  drv_Key_Init();
  KeyEvent_e queueMsg;

  for(;;)
  {
      drv_Key_Scan(); // 运行一次状态机

      if (Key1_GetEvent() == KEY_EVENT_PRESS)
      {
          queueMsg = KEY_PAGE_SWITCH;
          //xQueueSend 向队列（Queue）中发送数据。
          //xKeyQueue 告诉操作系统，你要把数据塞进哪一个队列里。
          //&queueMsg (信件内容)
          //0 (阻塞超时时间) 填 0 意味着非阻塞发送。
          //在 RTOS 的经典架构中，这叫 “生产者 - 消费者” 模型。
          //StartKeyScanTask 是生产者：它只管无脑地扫描硬件，发现了动作就往 xKeyQueue 里狂塞数据。
          //StartDisplayTask 是消费者：它就是那个一直在查询（掏信箱）的任务！
          xQueueSend(xKeyQueue, &queueMsg, 0);
          Debug_Printf("[KeyTask] 按键1: 切换页面\r\n");
          Key1_ClearEvent(); // 清除标志
      }

      if (Key2_GetEvent() == KEY_EVENT_PRESS)
      {
          queueMsg = KEY_PLUS;
          xQueueSend(xKeyQueue, &queueMsg, 0);
          Debug_Printf("[KeyTask] 按键2: PLUS\r\n");
          Key2_ClearEvent();
      }

      if (Key3_GetEvent() == KEY_EVENT_PRESS)
      {
          queueMsg = KEY_MINUS;
          xQueueSend(xKeyQueue, &queueMsg, 0);
          Debug_Printf("[KeyTask] 按键3: MINUS\r\n");
          Key3_ClearEvent();
      }

      osDelay(20); // 配合消抖时间，完美轮询
  }
  /* USER CODE END StartKeyScanTask */
}

/* USER CODE BEGIN Header_StartLogTask */
/**
* @brief Function implementing the LogTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartLogTask */
void StartLogTask(void const * argument)
{
  /* USER CODE BEGIN StartLogTask */
  SensorData_t logData;

  for(;;)
  {
      // 1. 获取系统当前的最新一帧传感器数据
      logData = g_LatestSensorData;

      // 2. 打上当前系统的 Tick 作为时间戳
      logData.timestamp = xTaskGetTickCount();

      // 3. 安全地写入环形缓冲区 (内部已加互斥锁)
      RingBuffer_Write(&logData);

      // 测试打印 (调试确认没问题后可以屏蔽这句)
      Debug_Printf("[LogTask] 记录历史数据 1 条，当前Tick: %lu\r\n", logData.timestamp);

      // 4. 任务运行周期：1 秒 (1000ms)
      osDelay(1000);
  }
  /* USER CODE END StartLogTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
#if 0
void StartLEDTask(void const * argument)
{
  for(;;)
  {
    HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
    osDelay(1000);
  }
}
#endif
void StartUARTTask(void const * argument)
{
  /* USER CODE BEGIN StartUARTTask */
  uint8_t rx_byte;//暂存单字符
  char cmd_buffer[64];//准备好的局部字符串数组（缓冲区）。最大能装 64 个字符。
  uint8_t cmd_idx = 0;//：数组的索引/游标。记录当前“篮子”里已经装了几个字符了

  Debug_Printf("\r\n=== UART Task Ready (Bulletproof Mode) ===\r\n");

  // 1. 🟢 强制解锁，打碎所有初始卡死的可能！
  huart1.Lock = HAL_UNLOCKED;
  __HAL_UART_CLEAR_OREFLAG(&huart1);
  huart1.ErrorCode = HAL_UART_ERROR_NONE;
  huart1.RxState = HAL_UART_STATE_READY;

  for(;;)
  {
      // 2. 挂起任务，死等第 1 个字符 (此时不占用任何 CPU，按键无比丝滑)
	  //HAL_UART_Receive：STM32 官方提供的阻塞式串口接收函数。
	  //参数 1 &huart1：传入串口 1 的配置句柄指针。
	  //参数 2 &rx_byte：告诉函数，收到数据后存到 rx_byte 这个变量的内存地址里。
	  //参数 3 1：期待接收的数据长度。这里写 1，代表只等 1 个字符。
	  //参数 4 portMAX_DELAY：【灵魂参数】这是 FreeRTOS 的宏。它告诉操作系统：“如果没收到这 1 个字，就把这个任务永远踢出 CPU 去休眠睡觉”。这保证了串口空闲时，毫不拖累你的按键和传感器任务。
	  //逻辑：只有当真正的字符到来，且函数返回 HAL_OK 时，才会往下执行。
      if (HAL_UART_Receive(&huart1, &rx_byte, 1, portMAX_DELAY) == HAL_OK)
      {
          // 3. 🟢 只要收到新字符，强制清空旧的乱码残骸！绝不缝合！
          cmd_idx = 0;

          if (rx_byte >= 32 && rx_byte <= 126) {
              cmd_buffer[cmd_idx++] = rx_byte;
          }

          // 4. 🟢 核心黑科技：既然电脑开始发数据了，立刻绕过 RTOS，全速抓取剩余字符！
          //设计逻辑：既然对方已经开口说话了（发了第一个字），后续的字肯定会以微秒级的间隔接踵而至。
          //这时候绝不能再调用臃肿的 HAL_UART_Receive，而是进入死循环，直接去硬件底层抢数据。
          uint32_t empty_loops = 0;
          while (1)
          {
              // 直接读取底层寄存器，速度快于 115200 波特率百倍，绝不会发生丢帧！
        	  //__HAL_UART_GET_FLAG：读取串口硬件状态寄存器（SR/ISR）的指定标志位。
        	  //参数 UART_FLAG_RXNE：RX Not Empty（接收寄存器非空）。只要硬件接收引脚上收满了一个字节，这个标志位就会被硬件自动置 1（不等于 RESET）。
              if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET)
              {
            	  //Instance 指向串口的硬件基地址，DR 是最底层的数据寄存器（Data Register）。
            	  //& 0x00FF 是为了屏蔽高 8 位，只取低 8 位有效数据。提取完后，顺手把超时计数器清零。
            	  //STM32 是一款 32 位的单片机。它的底层硬件寄存器（包括这个 DR 数据寄存器）在物理电路上都是 32 位的。
            	  //我们常用的串口通信（比如标准的 ASCII 码文本通信）每次传输的数据帧通常只有 8 位（1个字节）。
                  rx_byte = (uint8_t)(huart1.Instance->DR & 0x00FF);
                  empty_loops = 0; // 抓到数据，刷新超时计数

                  if (rx_byte == '\r' || rx_byte == '\n' || rx_byte == '#') {
                      break; // 收到结束符，完美跳出
                  }
                  else if (rx_byte >= 32 && rx_byte <= 126) {
                	  //判断是否会溢出
                      if (cmd_idx < sizeof(cmd_buffer) - 1) {
                          cmd_buffer[cmd_idx++] = rx_byte;
                      }
                  }
              }
              else
              {
                  // 约等待 2 毫秒。如果 2 毫秒都没新字来，说明这句指令结束了
            	  //以 115200 波特率算，传一个字只要 0.08 毫秒
                  empty_loops++;
                  if (empty_loops > 50000) {
                      break;
                  }
              }

              // 顺手防御底层的意外硬件溢出
              //在底层狂飙时，如果有瞬间的电磁脉冲导致硬件塞车报错（ORE 标志置位），利用 __HAL_UART_CLEAR_OREFLAG 顺手把它清掉，确保下一次接收正常。
              if (__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET) {
                  __HAL_UART_CLEAR_OREFLAG(&huart1);
              }
          }

          // 5. 完整抓取完毕，执行解析
          if (cmd_idx > 0)
          {
              cmd_buffer[cmd_idx] = '\0';
              Debug_Printf("\r\n[Cmd Parsed]: %s\r\n", cmd_buffer);
              Cmd_Parse(cmd_buffer);
          }
      }
      else
      {	//这个 else 对应的是最外层 HAL_UART_Receive 返回非 HAL_OK 的情况。
          // 6. ：一旦报错，彻底解锁 HAL 库，按键永不死机！
          __HAL_UART_CLEAR_OREFLAG(&huart1);
          huart1.Lock = HAL_UNLOCKED;       // 这句是解救按键的钥匙！
          huart1.ErrorCode = HAL_UART_ERROR_NONE;
          huart1.RxState = HAL_UART_STATE_READY;
          osDelay(5);
      }
  }
}
#if 0
void StartSensorTask(void const * argument)
{
    SensorData_t data;
    for(;;)
    {
        // 模拟数据
        data.temperature = 25.0;
        data.humidity = 60.0;
        data.smoke = 0.12;
        data.timestamp = xTaskGetTickCount();
        Debug_Printf("SensorTask: T=%.1f H=%.1f S=%.2f\r\n", data.temperature, data.humidity, data.smoke);
        xQueueSend(xSensorQueue, &data, 0);
        osDelay(2000);
    }
}
#endif
/* USER CODE END Application */

