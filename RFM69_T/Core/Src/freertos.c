/* USER CODE BEGIN Header */
/**
	******************************************************************************
	* File Name          : freertos.c
	* Description        : Code for freertos applications
	******************************************************************************
	* @attention
	*
	* <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
	* All rights reserved.</center></h2>
	*
	* This software component is licensed by ST under Ultimate Liberty license
	* SLA0044, the "License"; You may not use this file except in compliance with
	* the License. You may obtain a copy of the License at:
	*                             www.st.com/SLA0044
	*
	******************************************************************************
	*/
//
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <queue.h>
#include <semphr.h>

#include "MyAppConfig.h"
#include "RFM69_Config.h"
#include <stm32f4xx_hal_dma.h>
#include "RFM69_.h"
#include "RF69_ACK.h"
#include "RND_GEN.h"
#include "AES_GEN.h"
#include "AES_KEY.h"
#include "TIMER_C.h"
#include "ssd1306.h"
#include "RFM69_FreeRTOS.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
typedef StaticTask_t osStaticThreadDef_t;
typedef StaticQueue_t osStaticMessageQDef_t;
typedef StaticSemaphore_t osStaticMutexDef_t;
typedef StaticSemaphore_t osStaticSemaphoreDef_t;
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
/******************************************************************************/

volatile static uint32_t _good = 0;
volatile static uint32_t _bad = 0;
volatile static uint32_t _all = 0;
volatile static float _percent = 0;
//volatile static uint16_t _DioPin = 0;
volatile static bool _BufferRX_Valid = false;
/******************************************************************************/
//extern bool _rxBufValid;
/******************************************************************************/

#if _APP_MODE == 1U //Fixed Packet
extern const uint8_t _LMessage[256];
uint8_t buf[RF69_MAX_FIFO_LENGHT];
#endif
#if _APP_MODE == 2U //Var Packet
extern const uint8_t _LMessage[256];
uint8_t buf[256];
#endif
#if _APP_MODE == 3U //Unlim Packet
extern const uint8_t  _UMessage[512];
uint8_t buf[512];
#endif
#if _APP_MODE == 4U //Fixed Packet with ACK
extern const uint8_t _LMessage[256];
uint8_t buf[RF69_MAX_FIFO_LENGHT];
#endif
#if _APP_MODE == 5U //Var Packet with ACK
extern const uint8_t _LMessage[256];
uint8_t buf[256];
#endif
#if _APP_MODE == 6U //Unlim Packet with ACK
extern const uint8_t  _UMessage[512];
uint8_t buf[512];


volatile static uint8_t EncryptMessage[PLAINTEXT_LENGTH];
volatile static uint8_t OutputMessage[PLAINTEXT_LENGTH];
volatile static uint32_t OutputMessageLength = 0;

extern SPI_HandleTypeDef _SPI_HANDLE;
extern DMA_HandleTypeDef _SPI_DMA_RX_HANDLE;
extern DMA_HandleTypeDef _SPI_DMA_TX_HANDLE;
extern TIM_HandleTypeDef _TIMER_WATCH_HANDLE;
extern DMA_HandleTypeDef hdma_spi2_rx;
#endif

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
uint32_t defaultTaskBuffer[ 128 ];
osStaticThreadDef_t defaultTaskControlBlock;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .cb_mem = &defaultTaskControlBlock,
  .cb_size = sizeof(defaultTaskControlBlock),
  .stack_mem = &defaultTaskBuffer[0],
  .stack_size = sizeof(defaultTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for InitTask */
osThreadId_t InitTaskHandle;
const osThreadAttr_t InitTask_attributes = {
  .name = "InitTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for RadioTask */
osThreadId_t RadioTaskHandle;
uint32_t RadioTaskBuffer[ 512 ];
osStaticThreadDef_t RadioTaskControlBlock;
const osThreadAttr_t RadioTask_attributes = {
  .name = "RadioTask",
  .cb_mem = &RadioTaskControlBlock,
  .cb_size = sizeof(RadioTaskControlBlock),
  .stack_mem = &RadioTaskBuffer[0],
  .stack_size = sizeof(RadioTaskBuffer),
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TaskKey */
osThreadId_t TaskKeyHandle;
const osThreadAttr_t TaskKey_attributes = {
  .name = "TaskKey",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for TaskDIO_RF69 */
osThreadId_t TaskDIO_RF69Handle;
uint32_t DIO_RF69Buffer[ 256 ];
osStaticThreadDef_t DIO_RF69ControlBlock;
const osThreadAttr_t TaskDIO_RF69_attributes = {
  .name = "TaskDIO_RF69",
  .cb_mem = &DIO_RF69ControlBlock,
  .cb_size = sizeof(DIO_RF69ControlBlock),
  .stack_mem = &DIO_RF69Buffer[0],
  .stack_size = sizeof(DIO_RF69Buffer),
  .priority = (osPriority_t) osPriorityRealtime,
};
/* Definitions for TaskTIM_RF69 */
osThreadId_t TaskTIM_RF69Handle;
uint32_t TaskTIM_RF69Buffer[ 256 ];
osStaticThreadDef_t TaskTIM_RF69ControlBlock;
const osThreadAttr_t TaskTIM_RF69_attributes = {
  .name = "TaskTIM_RF69",
  .cb_mem = &TaskTIM_RF69ControlBlock,
  .cb_size = sizeof(TaskTIM_RF69ControlBlock),
  .stack_mem = &TaskTIM_RF69Buffer[0],
  .stack_size = sizeof(TaskTIM_RF69Buffer),
  .priority = (osPriority_t) osPriorityRealtime1,
};
/* Definitions for TaskRF69_Recive */
osThreadId_t TaskRF69_ReciveHandle;
uint32_t TaskRF69_ReciveBuffer[ 256 ];
osStaticThreadDef_t TaskRF69_ReciveControlBlock;
const osThreadAttr_t TaskRF69_Recive_attributes = {
  .name = "TaskRF69_Recive",
  .cb_mem = &TaskRF69_ReciveControlBlock,
  .cb_size = sizeof(TaskRF69_ReciveControlBlock),
  .stack_mem = &TaskRF69_ReciveBuffer[0],
  .stack_size = sizeof(TaskRF69_ReciveBuffer),
  .priority = (osPriority_t) osPriorityNormal1,
};
/* Definitions for QueueBufRxValid */
osMessageQueueId_t QueueBufRxValidHandle;
uint8_t QueueBufRxValidBuffer[ 1 * sizeof( uint8_t ) ];
osStaticMessageQDef_t QueueBufRxValidControlBlock;
const osMessageQueueAttr_t QueueBufRxValid_attributes = {
  .name = "QueueBufRxValid",
  .cb_mem = &QueueBufRxValidControlBlock,
  .cb_size = sizeof(QueueBufRxValidControlBlock),
  .mq_mem = &QueueBufRxValidBuffer,
  .mq_size = sizeof(QueueBufRxValidBuffer)
};
/* Definitions for RF_Mutex */
osMutexId_t RF_MutexHandle;
osStaticMutexDef_t RF_MutexControlBlock;
const osMutexAttr_t RF_Mutex_attributes = {
  .name = "RF_Mutex",
  .cb_mem = &RF_MutexControlBlock,
  .cb_size = sizeof(RF_MutexControlBlock),
};
/* Definitions for SemaphoreKEY */
osSemaphoreId_t SemaphoreKEYHandle;
const osSemaphoreAttr_t SemaphoreKEY_attributes = {
  .name = "SemaphoreKEY"
};
/* Definitions for SemDIO_RF69 */
osSemaphoreId_t SemDIO_RF69Handle;
osStaticSemaphoreDef_t SemDIO_RF69ControlBlock;
const osSemaphoreAttr_t SemDIO_RF69_attributes = {
  .name = "SemDIO_RF69",
  .cb_mem = &SemDIO_RF69ControlBlock,
  .cb_size = sizeof(SemDIO_RF69ControlBlock),
};
/* Definitions for SemTimer_RF69 */
osSemaphoreId_t SemTimer_RF69Handle;
osStaticSemaphoreDef_t SemTimer_RF69ControlBlock;
const osSemaphoreAttr_t SemTimer_RF69_attributes = {
  .name = "SemTimer_RF69",
  .cb_mem = &SemTimer_RF69ControlBlock,
  .cb_size = sizeof(SemTimer_RF69ControlBlock),
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void PrintText(uint8_t* pBuf, uint8_t lenght);
//void EXTI0_IRQHandler(void);
void TIM_IRQHandler(TIM_HandleTypeDef *htim);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartInitTask(void *argument);
void StartRadioTask(void *argument);
void StartTaskKey(void *argument);
void StartDIO_RF69(void *argument);
void StartTaskTIM_RF69(void *argument);
void StartTaskRF69_Recive(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of RF_Mutex */
  RF_MutexHandle = osMutexNew(&RF_Mutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
	/* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of SemaphoreKEY */
  SemaphoreKEYHandle = osSemaphoreNew(1, 1, &SemaphoreKEY_attributes);

  /* creation of SemDIO_RF69 */
  SemDIO_RF69Handle = osSemaphoreNew(1, 1, &SemDIO_RF69_attributes);

  /* creation of SemTimer_RF69 */
  SemTimer_RF69Handle = osSemaphoreNew(1, 1, &SemTimer_RF69_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
	/* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of QueueBufRxValid */
  QueueBufRxValidHandle = osMessageQueueNew (1, sizeof(uint8_t), &QueueBufRxValid_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of InitTask */
  InitTaskHandle = osThreadNew(StartInitTask, NULL, &InitTask_attributes);

  /* creation of RadioTask */
  RadioTaskHandle = osThreadNew(StartRadioTask, NULL, &RadioTask_attributes);

  /* creation of TaskKey */
  TaskKeyHandle = osThreadNew(StartTaskKey, NULL, &TaskKey_attributes);

  /* creation of TaskDIO_RF69 */
  TaskDIO_RF69Handle = osThreadNew(StartDIO_RF69, NULL, &TaskDIO_RF69_attributes);

  /* creation of TaskTIM_RF69 */
  TaskTIM_RF69Handle = osThreadNew(StartTaskTIM_RF69, NULL, &TaskTIM_RF69_attributes);

  /* creation of TaskRF69_Recive */
  TaskRF69_ReciveHandle = osThreadNew(StartTaskRF69_Recive, NULL, &TaskRF69_Recive_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
	/* Infinite loop */
	for(;;)
	{
		vTaskDelay(1);
	}
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartInitTask */
/**
* @brief Function implementing the InitTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartInitTask */
void StartInitTask(void *argument)
{
  /* USER CODE BEGIN StartInitTask */
  /* Infinite loop */
	osMutexWait(RF_MutexHandle, osWaitForever);
	RF69_Init();
	InitRND();
	
	
	
	osMutexRelease(RF_MutexHandle);
	AES_CTR_Set_Key(AK_Key128, CRL_AES128_KEY);
	 vTaskDelete(InitTaskHandle);
  /* USER CODE END StartInitTask */
}

/* USER CODE BEGIN Header_StartRadioTask */
/**
* @brief Function implementing the RadioTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartRadioTask */
void StartRadioTask(void *argument)
{
  /* USER CODE BEGIN StartRadioTask */
	/* Infinite loop */
	
	volatile static bool _flag= 0;
	for(;;)
	{
		osMutexWait(RF_MutexHandle, osWaitForever);
#if _APP_MODE == 1U //Fixed Packet
		RF69_PacketMode(_PACKET_FIXED);
		_flag = RF69_Send ((uint8_t*)_LMessage, 28);
		_all++;
		if(_flag)
		{
			LED_GREEN_ON;
			_good++;
		}
		else
		{
			LED_RED_ON;
			_bad;
		}
		_percent = _bad/((float)_all/100);

#endif
#if _APP_MODE == 2U //Variable Packet
//		_flag = RF69_WaitAvailableTimeout(100);
		const  uint8_t size = 253;
		_all++;
		RF69_PacketMode(_PACKET_VARIABLE);
		RF69_SendVariablePacket ((uint8_t*)_LMessage, size);
		if(_flag)
		{
			LED_GREEN_ON;
			_good++;
		}
		else
		{
			LED_RED_ON;
			_bad;
		}
		_percent = _bad/((float)_all/100);
//extern bool SendACK(uint8_t *buf, uint16_t len);

//	SendACK((uint8_t*)_LMessage, size);
		
#endif
	
#if _APP_MODE == 3U //Unlimited Packet
//		_flag = RF69_WaitAvailableTimeout(100);
		_all++;
		RF69_SendUnlimitedLengthPacket ((uint8_t*)_UMessage, sizeof(_UMessage));
		if(_flag)
		{
			LED_GREEN_ON;
			_good++;
		}
		else
		{
			LED_RED_ON;
			_bad;
		}
		_percent = _bad/((float)_all/100);
        
#endif
#if _APP_MODE == 4U //Fix Packet with ACK
		_all++;
		_flag = SendFixACK((uint8_t*)_LMessage, 24);
		if(_flag)
		{
			LED_GREEN_ON;
			_good++;
		}
		else
		{
			LED_RED_ON;
			_bad++;
		}
		
#endif
		
#if _APP_MODE == 5U //Var Packet with ACK
		_all++;
		_flag = SendVarACK((uint8_t*) _LMessage, 230);
		if(_flag)
		{
			LED_GREEN_ON;
			_good++;
		}
		else
		{
			LED_RED_ON;
			_bad++;
		}
#endif
#if _APP_MODE == 6U //Unlim Packet with ACK
		_all++;
		_flag = SendUnlimACK((uint8_t*) _UMessage, 500);
		if(_flag)
		{
			LED_GREEN_ON;
			_good++;
		}
		else
		{
			LED_RED_ON;
			_bad++;
		}
#endif
/******************************************************************************/
		osMutexRelease(RF_MutexHandle);

//		_timer_c_StartTimer(10000);
		 vTaskDelay(20);

		LED_BLUE_OFF;
		LED_GREEN_OFF;
		LED_RED_OFF;
		LED_YELL_OFF;
		vTaskDelay(300);
		
		
 }

  /* USER CODE END StartRadioTask */
}

/* USER CODE BEGIN Header_StartTaskKey */
/**
* @brief Function implementing the TaskKey thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskKey */
void StartTaskKey(void *argument)
{
  /* USER CODE BEGIN StartTaskKey */
  /* Infinite loop */
  for(;;)
  {
	  xSemaphoreTake(SemaphoreKEYHandle, portMAX_DELAY );
	  osDelay(100);
	  if(HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin) == GPIO_PIN_SET)
	  {
		  LED1_ON;
		  osDelay(20);
		  LED1_OFF;
//		  osDelay(180);
		  DBG_ON;
	  }
	  else
	  {
		  LED1_ON;
		  osDelay(20);
		  LED1_OFF;
//		  osDelay(180);
//		  LED1_OFF;
		  DBG_OFF;
	  }
  }
  /* USER CODE END StartTaskKey */
}

/* USER CODE BEGIN Header_StartDIO_RF69 */
/**
* @brief Function implementing the TaskDIO_RF69 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDIO_RF69 */
__weak void StartDIO_RF69(void *argument)
{
  /* USER CODE BEGIN StartDIO_RF69 */
	/* Infinite loop */
//	static portBASE_TYPE xHigherPriorityTaskWoken;
//	xHigherPriorityTaskWoken = pdFALSE;
	for(;;)
	{

		osDelay(1);
	}
  /* USER CODE END StartDIO_RF69 */
}

/* USER CODE BEGIN Header_StartTaskTIM_RF69 */
/**
* @brief Function implementing the TaskTIM_RF69 thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskTIM_RF69 */
void StartTaskTIM_RF69(void *argument)
{
  /* USER CODE BEGIN StartTaskTIM_RF69 */
	/* Infinite loop */
	for(;;)
	{
		xSemaphoreTake(SemDIO_RF69Handle, portMAX_DELAY );
		RF69_CallbackWatchTimer();
//		osDelay(1);
	}
  /* USER CODE END StartTaskTIM_RF69 */
}

/* USER CODE BEGIN Header_StartTaskRF69_Recive */
/**
* @brief Function implementing the TaskRF69_Recive thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTaskRF69_Recive */
__weak void StartTaskRF69_Recive(void *argument)
{
  /* USER CODE BEGIN StartTaskRF69_Recive */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartTaskRF69_Recive */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	static portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
///* Prevent unused argument(s) compilation warning */

	RF69_OS_GPIO_EXTI_Callback(GPIO_Pin);
	if(GPIO_Pin == KEY_Pin)
	{ 
		xSemaphoreGiveFromISR(SemaphoreKEYHandle, &xHigherPriorityTaskWoken );
		portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	}
	UNUSED(GPIO_Pin);
 
}

void TIM_IRQHandler(TIM_HandleTypeDef *htim)
{
	static portBASE_TYPE xHigherPriorityTaskWoken;
	xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(SemTimer_RF69Handle, &xHigherPriorityTaskWoken );
	portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
	UNUSED(htim);
}
void PrintText(uint8_t* pBuf, uint8_t lenght)
{
}
/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
