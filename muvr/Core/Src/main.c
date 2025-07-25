/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <math.h>
#include "BME280_STM32.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART_TX_BUF_SIZE 256
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c3;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_tx;

/* Definitions for CoMeasureTask */
osThreadId_t CoMeasureTaskHandle;
const osThreadAttr_t CoMeasureTask_attributes = {
  .name = "CoMeasureTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for WeatherTask */
osThreadId_t WeatherTaskHandle;
const osThreadAttr_t WeatherTask_attributes = {
  .name = "WeatherTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Speaker */
osThreadId_t SpeakerHandle;
const osThreadAttr_t Speaker_attributes = {
  .name = "Speaker",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for FanTask */
osThreadId_t FanTaskHandle;
const osThreadAttr_t FanTask_attributes = {
  .name = "FanTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for UartCommand */
osThreadId_t UartCommandHandle;
const osThreadAttr_t UartCommand_attributes = {
  .name = "UartCommand",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for UartDisplay */
osThreadId_t UartDisplayHandle;
const osThreadAttr_t UartDisplay_attributes = {
  .name = "UartDisplay",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for WatchdogTask */
osThreadId_t WatchdogTaskHandle;
const osThreadAttr_t WatchdogTask_attributes = {
  .name = "WatchdogTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityHigh,
};
/* Definitions for queue_r0 */
osMessageQueueId_t queue_r0Handle;
const osMessageQueueAttr_t queue_r0_attributes = {
  .name = "queue_r0"
};
/* Definitions for queue_pr */
osMessageQueueId_t queue_prHandle;
const osMessageQueueAttr_t queue_pr_attributes = {
  .name = "queue_pr"
};
/* Definitions for queue_co */
osMessageQueueId_t queue_coHandle;
const osMessageQueueAttr_t queue_co_attributes = {
  .name = "queue_co"
};
/* Definitions for uartCommandQueue */
osMessageQueueId_t uartCommandQueueHandle;
const osMessageQueueAttr_t uartCommandQueue_attributes = {
  .name = "uartCommandQueue"
};
/* Definitions for queue_watchdog */
osMessageQueueId_t queue_watchdogHandle;
const osMessageQueueAttr_t queue_watchdog_attributes = {
  .name = "queue_watchdog"
};
/* Definitions for xUARTMutex */
osMutexId_t xUARTMutexHandle;
const osMutexAttr_t xUARTMutex_attributes = {
  .name = "xUARTMutex"
};
/* Definitions for xFANMutex */
osMutexId_t xFANMutexHandle;
const osMutexAttr_t xFANMutex_attributes = {
  .name = "xFANMutex"
};
/* Definitions for xSpeakerSem */
osSemaphoreId_t xSpeakerSemHandle;
const osSemaphoreAttr_t xSpeakerSem_attributes = {
  .name = "xSpeakerSem"
};
/* USER CODE BEGIN PV */
float Temperature, Pressure, Humidity;
WeatherData_t data;
uint8_t rx_buff = 0;
volatile uint8_t fan_continuous_mode = 0;


static uint8_t uart_tx_buffer[UART_TX_BUF_SIZE];

static volatile uint16_t uart_tx_head = 0;
static volatile uint16_t uart_tx_tail = 0;
static volatile uint8_t uart_tx_busy = 0;

static uint16_t current_transmit_length = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
static void MX_ADC1_Init(void);
static void MX_I2C3_Init(void);
static void MX_TIM1_Init(void);
void co_measure_task(void *argument);
void weather_task(void *argument);
void speaker_alarm_task(void *argument);
void fan_task(void *argument);
void uart_command_task(void *argument);
void uart_display_task(void *argument);
void watch_dog_task(void *argument);

/* USER CODE BEGIN PFP */
void Speaker_Beep(uint32_t duration_ms);
void fan_rotating(uint32_t duration_ms);
void BME280_Init(void);
void fan_stop(void);
//void DMATransferComplete(DMA_HandleTypeDef *hdma);
int uart_tx_enqueue(const uint8_t* data, uint16_t len);
void uart_start_tx_dma(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART2_UART_Init();
  MX_TIM2_Init();
  MX_ADC1_Init();
  MX_I2C3_Init();
  MX_TIM1_Init();
  /* USER CODE BEGIN 2 */
  BME280_Config(OSRS_2, OSRS_16, OSRS_1, MODE_NORMAL, T_SB_0p5, IIR_16);
//  HAL_DMA_RegisterCallback(&hdma_usart2_tx, HAL_DMA_XFER_CPLT_CB_ID, &DMATransferComplete);
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();
  /* Create the mutex(es) */
  /* creation of xUARTMutex */
  xUARTMutexHandle = osMutexNew(&xUARTMutex_attributes);

  /* creation of xFANMutex */
  xFANMutexHandle = osMutexNew(&xFANMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of xSpeakerSem */
  xSpeakerSemHandle = osSemaphoreNew(1, 0, &xSpeakerSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of queue_r0 */
  queue_r0Handle = osMessageQueueNew (5, sizeof(WeatherData_t), &queue_r0_attributes);

  /* creation of queue_pr */
  queue_prHandle = osMessageQueueNew (5, sizeof(float), &queue_pr_attributes);

  /* creation of queue_co */
  queue_coHandle = osMessageQueueNew (5, sizeof(float), &queue_co_attributes);

  /* creation of uartCommandQueue */
  uartCommandQueueHandle = osMessageQueueNew (3, sizeof(uint8_t), &uartCommandQueue_attributes);

  /* creation of queue_watchdog */
  queue_watchdogHandle = osMessageQueueNew (4, sizeof(WatchdogError_t), &queue_watchdog_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of CoMeasureTask */
  CoMeasureTaskHandle = osThreadNew(co_measure_task, NULL, &CoMeasureTask_attributes);

  /* creation of WeatherTask */
  WeatherTaskHandle = osThreadNew(weather_task, NULL, &WeatherTask_attributes);

  /* creation of Speaker */
  SpeakerHandle = osThreadNew(speaker_alarm_task, NULL, &Speaker_attributes);

  /* creation of FanTask */
  FanTaskHandle = osThreadNew(fan_task, NULL, &FanTask_attributes);

  /* creation of UartCommand */
  UartCommandHandle = osThreadNew(uart_command_task, NULL, &UartCommand_attributes);

  /* creation of UartDisplay */
  UartDisplayHandle = osThreadNew(uart_display_task, NULL, &UartDisplay_attributes);

  /* creation of WatchdogTask */
  WatchdogTaskHandle = osThreadNew(watch_dog_task, NULL, &WatchdogTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C3_Init(void)
{

  /* USER CODE BEGIN I2C3_Init 0 */

  /* USER CODE END I2C3_Init 0 */

  /* USER CODE BEGIN I2C3_Init 1 */

  /* USER CODE END I2C3_Init 1 */
  hi2c3.Instance = I2C3;
  hi2c3.Init.ClockSpeed = 100000;
  hi2c3.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c3.Init.OwnAddress1 = 0;
  hi2c3.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c3.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c3.Init.OwnAddress2 = 0;
  hi2c3.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c3.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 83;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 19999;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 499;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 5, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8|GPIO_PIN_12|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14, GPIO_PIN_RESET);

  /*Configure GPIO pins : PE8 PE12 PE14 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_12|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pins : PD12 PD13 PD14 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

int uart_tx_enqueue(const uint8_t* data, uint16_t len) {
    uint16_t next_head;
    for (uint16_t i = 0; i < len; i++) {
        next_head = (uart_tx_head + 1) % UART_TX_BUF_SIZE;
        if (next_head == uart_tx_tail) {
            // Bafer pun
            return -1;
        }
        uart_tx_buffer[uart_tx_head] = data[i];
        uart_tx_head = next_head;
    }
    return 0;
}

void uart_start_tx_dma(void) {
    if (uart_tx_busy) return; // već radi

    if (uart_tx_head == uart_tx_tail) {
        // Bafer prazan
        return;
    }

    uart_tx_busy = 1;

    if (uart_tx_head > uart_tx_tail) {
        current_transmit_length = uart_tx_head - uart_tx_tail;
    } else {
        current_transmit_length = UART_TX_BUF_SIZE - uart_tx_tail;
    }

    HAL_UART_Transmit_DMA(&huart2, &uart_tx_buffer[uart_tx_tail], current_transmit_length);
}

void Speaker_Beep(uint32_t duration_ms)
{
    htim2.Instance->CCR2 = 470;
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    osDelay(pdMS_TO_TICKS(duration_ms));
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
}

void fan_rotating(uint32_t duration_ms)
{
	HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_GPIO_Pin, GPIO_PIN_RESET);
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 19000);
	osMutexAcquire(xUARTMutexHandle, osWaitForever);
	HAL_UART_Transmit(&huart2, (uint8_t*)"F\r\n", 3, 100);
	osMutexRelease(xUARTMutexHandle);
    osDelay(pdMS_TO_TICKS(duration_ms));
	fan_stop();
}

void fan_stop(void)
{
	__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 0);
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        osMessageQueuePut(uartCommandQueueHandle, &rx_buff, 0, 0);
        HAL_UART_Receive_IT(&huart2, &rx_buff, 1);
    }
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        uart_tx_tail = (uart_tx_tail + current_transmit_length) % UART_TX_BUF_SIZE;

        if (uart_tx_tail != uart_tx_head) {
            uart_tx_busy = 0;
            uart_start_tx_dma();
        } else {
            uart_tx_busy = 0;
        }
    }
}



/* USER CODE END 4 */

/* USER CODE BEGIN Header_co_measure_task */
/**
  * @brief  Function implementing the CoMeasureTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_co_measure_task */
void co_measure_task(void *argument)
{
  /* USER CODE BEGIN 5 */
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
	uint32_t adc_value;
	const float Vcc = 5.0f;
	const float RL = 10000.0f;
	WeatherData_t latestWeather = { .temperature = 25.0f, .humidity = 52.0f };
// Kalibracija R0 za trenutnu temp i hum
//	osDelay(pdMS_TO_TICKS(1000));
	float Vout;
	float Rs, T, H;
	osDelay(pdMS_TO_TICKS(100));
	osMessageQueueGet(queue_r0Handle, &latestWeather, NULL, osWaitForever);
	T = latestWeather.temperature;
	H = latestWeather.humidity;
	HAL_ADC_Start(&hadc1);
	if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
		adc_value = HAL_ADC_GetValue(&hadc1);
	}
	HAL_ADC_Stop(&hadc1);

	Vout = (adc_value * Vcc) / 4095.0f;
	Rs = ((Vcc - Vout) * RL) / Vout;


	float R0 = Rs / (-0.0122f * T - 0.00609f * H + 1.7086f);

	char msg[64];
	snprintf(msg, sizeof(msg), "R0 = %.2f\r\n (T=%.1fC H=%.1f%%)", R0, T, H);
	osMutexAcquire(xUARTMutexHandle, osWaitForever);
	HAL_UART_Transmit_DMA(&huart2, (uint8_t*)msg, strlen(msg));
	osMutexRelease(xUARTMutexHandle);


	// Petlja za merenje CO
	for (;;) {
		HAL_ADC_Start(&hadc1);
		if (HAL_ADC_PollForConversion(&hadc1, 100) == HAL_OK) {
			adc_value = HAL_ADC_GetValue(&hadc1);
			Vout = (adc_value * Vcc) / 4095.0f;
			Rs = ((Vcc - Vout) * RL) / Vout;

			float ratio = Rs / R0;
			float co_ppm = 19.709f * powf(ratio, -0.652f);


			osMessageQueuePut(queue_coHandle, &co_ppm, 0, pdMS_TO_TICKS(100));
			if (co_ppm > 30.0f) {
//			    HAL_UART_Transmit(&huart2, (uint8_t*)"Speaker Semafor RELEASE\r\n", 26, 100);
				xTaskNotifyGive(FanTaskHandle);
				osSemaphoreRelease(xSpeakerSemHandle);
			}
		}
		HAL_ADC_Stop(&hadc1);
		xTaskNotify(WatchdogTaskHandle, 0x01, eSetBits);
		osDelay(pdMS_TO_TICKS(500));
	}

  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_weather_task */
/**
* @brief Function implementing the WeatherTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_weather_task */
void weather_task(void *argument)
{
  /* USER CODE BEGIN weather_task */

	for (;;) {
		BME280_Measure();
		osMessageQueuePut(queue_r0Handle, &data, 0, pdMS_TO_TICKS(100));


		osMessageQueuePut(queue_prHandle, &Pressure, 0, pdMS_TO_TICKS(100));
		if(Pressure>1020000	){
		    osSemaphoreRelease(xSpeakerSemHandle);
		}
		xTaskNotify(WatchdogTaskHandle, 0x02, eSetBits);
		osDelay(pdMS_TO_TICKS(500));
	}
  /* USER CODE END weather_task */
}

/* USER CODE BEGIN Header_speaker_alarm_task */
/**
* @brief Function implementing the Speaker thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_speaker_alarm_task */
void speaker_alarm_task(void *argument)
{
  /* USER CODE BEGIN speaker_alarm_task */
//	HAL_UART_Transmit(&huart2, (uint8_t*)"Speaker task running...\r\n", 26, 100);
  /* Infinite loop */
  for(;;)
  {
	  osSemaphoreAcquire(xSpeakerSemHandle, osWaitForever);
//	  HAL_UART_Transmit(&huart2, (uint8_t*)"Beep!\r\n", 7, 100);
	  Speaker_Beep(1000);
	}

  /* USER CODE END speaker_alarm_task */
}

/* USER CODE BEGIN Header_fan_task */
/**
* @brief Function implementing the FanTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_fan_task */
void fan_task(void *argument)
{
  /* USER CODE BEGIN fan_task */
	  HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_GPIO_Pin, GPIO_PIN_SET);
	  HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_GPIO_Pin, GPIO_PIN_SET);
	  HAL_GPIO_WritePin(SLP_GPIO_Port, SLP_GPIO_Pin, GPIO_PIN_SET);
	  HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_3);
  /* Infinite loop */
  for(;;)
  {
	  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
//	  HAL_UART_Transmit(&huart2, (uint8_t*)"FAN\r\n", 5, 100);
	  if (fan_continuous_mode){
		  __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_3, 19000);
		  while (fan_continuous_mode)
		  {
			  osDelay(100);

			osMutexAcquire(xUARTMutexHandle, osWaitForever);
			HAL_UART_Transmit(&huart2, (uint8_t*)"F\r\n", 3, 100);
			osMutexRelease(xUARTMutexHandle);
		  }
		  osMutexAcquire(xFANMutexHandle, osWaitForever);
		  fan_stop();
		  osMutexRelease(xFANMutexHandle);
	  }else{
		  osMutexAcquire(xFANMutexHandle, osWaitForever);
		  fan_rotating(1000);
		  osMutexRelease(xFANMutexHandle);
	  }


  }
  /* USER CODE END fan_task */
}

/* USER CODE BEGIN Header_uart_command_task */
/**
* @brief Function implementing the UartCommand thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_uart_command_task */
void uart_command_task(void *argument)
{
  /* USER CODE BEGIN uart_command_task */
  /* Infinite loop */
	uint8_t fan_state;
	HAL_UART_Receive_IT(&huart2, &rx_buff, 1);
	for(;;)
	{
		if(osMessageQueueGet(uartCommandQueueHandle, &fan_state, NULL, osWaitForever) == osOK){
			if (fan_state == '0') {
			    fan_continuous_mode = 0;
				osMutexAcquire(xFANMutexHandle, osWaitForever);
			    fan_stop();
			    osMutexRelease(xFANMutexHandle);
			}
			else if (fan_state == '1') {
			    fan_continuous_mode = 1;
			    xTaskNotifyGive(FanTaskHandle);
			}
			else if (fan_state >= '2' && fan_state <= '9') {
			    fan_continuous_mode = 0;
			    uint32_t duration = (fan_state - '0') * 1000;
				osMutexAcquire(xFANMutexHandle, osWaitForever);
				fan_rotating(duration);
				osMutexRelease(xFANMutexHandle);
			}
		}

	}
  /* USER CODE END uart_command_task */
}

/* USER CODE BEGIN Header_uart_display_task */
/**
* @brief Function implementing the UartDisplay thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_uart_display_task */
void uart_display_task(void *argument)
{
  /* USER CODE BEGIN uart_display_task */
	float co_ppm;
	float Pressure;
	char msg[64];
	WatchdogError_t err;
  /* Infinite loop */
  for(;;)
  {

	  if(osMessageQueueGet(queue_coHandle, &co_ppm, NULL, osWaitForever) == osOK){
//		  while (huart2.gState != HAL_UART_STATE_READY) osDelay(1);
		  snprintf(msg, sizeof(msg), "UART: Co: %.1f ppm\r\n", co_ppm);
		  osMutexAcquire(xUARTMutexHandle, osWaitForever);
		  uart_tx_enqueue((uint8_t*)msg, strlen(msg));
		  uart_start_tx_dma();
//		  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)msg, strlen(msg));
		  osMutexRelease(xUARTMutexHandle);
	  }
	  if(osMessageQueueGet(queue_prHandle, &Pressure, NULL, osWaitForever) == osOK){
		  while (huart2.gState != HAL_UART_STATE_READY) osDelay(1);
		  snprintf(msg, sizeof(msg), "UART: P: %.2f hPa\r\n", Pressure);
		  osMutexAcquire(xUARTMutexHandle, osWaitForever);
//		  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)msg, strlen(msg));
		  uart_tx_enqueue((uint8_t*)msg, strlen(msg));
		  uart_start_tx_dma();
		  osMutexRelease(xUARTMutexHandle);
	  }
	  if (osMessageQueueGet(queue_watchdogHandle, &err, NULL, pdMS_TO_TICKS(100)) == osOK){
		  switch (err){
			case WTDG_ERROR_CO:
				snprintf(msg, sizeof(msg), "[WDTG] Greska u CO mjerenju\r\n");
				break;
			case WTDG_ERROR_WEATHER:
				snprintf(msg, sizeof(msg), "[WDTG] Greska u Weather mjerenju\r\n");
				break;
			case WTDG_ERROR_UART:
				snprintf(msg, sizeof(msg), "[WDTG] Greska u UART Prikazu\r\n");
				break;
			default:
				snprintf(msg, sizeof(msg), "[WDTG] Greska u CO mjerenju\r\n");
				break;
		}
		  osMutexAcquire(xUARTMutexHandle, osWaitForever);
		  while (huart2.gState != HAL_UART_STATE_READY) osDelay(1);
//		  HAL_UART_Transmit_DMA(&huart2, (uint8_t*)msg, strlen(msg));
		  uart_tx_enqueue((uint8_t*)msg, strlen(msg));
		  uart_start_tx_dma();
		  osMutexRelease(xUARTMutexHandle);
	  }

	  xTaskNotify(WatchdogTaskHandle, 0x04, eSetBits);  // uart task
	  osDelay(pdMS_TO_TICKS(300));
  }
  /* USER CODE END uart_display_task */
}

/* USER CODE BEGIN Header_watch_dog_task */
/**
* @brief Function implementing the WatchdogTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_watch_dog_task */
void watch_dog_task(void *argument)
{
  /* USER CODE BEGIN watch_dog_task */
	//bit mask co 0x01, weather 0x02, uart 0x04
	const TickType_t xDelay = pdMS_TO_TICKS(5000);
	uint32_t ulNotificationValue;
	uint32_t aliveMask;
	WatchdogError_t err;
	for (;;)
	{
		aliveMask = 0;
		TickType_t startTick = xTaskGetTickCount();
		while ((xTaskGetTickCount() - startTick) < xDelay)
				{
					if (xTaskNotifyWait(0x00, 0xFFFFFFFF, &ulNotificationValue, pdMS_TO_TICKS(500)) == pdTRUE)
					{
						aliveMask |= ulNotificationValue;
					}
				}
		if ((aliveMask & 0x07) == 0x07)
				{
					// Svi taskovi su se javili
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);   // ZELENA
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET); // CRVENA
				}else if (!(aliveMask & 0x01)) {
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET); // ZELENA
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);   // CRVENA
					err = WTDG_ERROR_CO;
					osMessageQueuePut(queue_watchdogHandle, &err, 0, pdMS_TO_TICKS(100));
				}else if (!(aliveMask & 0x02)) {
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET); // ZELENA
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);   // CRVENA
					err = WTDG_ERROR_WEATHER;
					osMessageQueuePut(queue_watchdogHandle, &err, 0, pdMS_TO_TICKS(100));


				}else if (!(aliveMask & 0x04)) {
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET); // ZELENA
					HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);   // CRVENA
					err = WTDG_ERROR_UART;
					osMessageQueuePut(queue_watchdogHandle, &err, 0, pdMS_TO_TICKS(100));
					}

			}



  /* USER CODE END watch_dog_task */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
