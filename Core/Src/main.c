/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "dma2d.h"
#include "ltdc.h"
#include "mdma.h"
#include "quadspi.h"
#include "rtc.h"
#include "sdmmc.h"
#include "usart.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#include "fatfs_port.h"
#include "bsp/board_api.h"
#include "qspi_w25q64.h"
#include "tusb_port.h"
#include "sdram.h"
#include "mem.h"
#include "freertos_port.h"
#include "sd.h"
#include "tlsf_port.h"
#include "lcd.h"
#include "uart.h"
#include "usb_msc.h"
#include "vendor.h"
#include "lcd.h"
#include "lv_port.h"
#include "lvgl.h"
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Zeller 公式计算星期几 (RTC编码: 1=Mon..7=Sun)
static int calc_weekday(int year, int month, int day)
{
    // Zeller's congruence (Gregorian calendar)
    if (month < 3) { month += 12; year--; }
    int k = year % 100;
    int j = year / 100;
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    if (h < 0) h += 7;
    // h: 0=Sat, 1=Sun, 2=Mon ... 6=Fri
    // RTC: 1=Mon, 2=Tue, 3=Wed, 4=Thu, 5=Fri, 6=Sat, 7=Sun
    static const int map[] = {6, 7, 1, 2, 3, 4, 5};
    return map[h];
}

// 解析 __DATE__/__TIME__ 编译时间写入RTC (编译时刻=启动时间)
static void set_rtc_from_build_time(void)
{
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    int hour, minute, second;
    sscanf(__TIME__, "%d:%d:%d", &hour, &minute, &second);
    sTime.Hours   = hour;
    sTime.Minutes = minute;
    sTime.Seconds = second;
    sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    sTime.StoreOperation = RTC_STOREOPERATION_RESET;

    char mon_str[4];
    int day, year;
    sscanf(__DATE__, "%3s %d %d", mon_str, &day, &year);

    int month = 1;
    for (int i = 0; i < 12; i++) {
        if (strncmp(mon_str, months[i], 3) == 0) {
            month = i + 1;
            break;
        }
    }

    sDate.WeekDay = calc_weekday(year, month, day);
    sDate.Month   = month;
    sDate.Date    = day;
    sDate.Year    = year - 2000;

    HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
}

#if BSP_FREERTOS_ENABLED
static StackType_t  led_task_stack[128];
static StaticTask_t led_task_tcb;
// LED 闪烁任务 (100ms 周期, FreeRTOS)
static void led_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static StackType_t  usb_task_stack[512];
static StaticTask_t usb_task_tcb;
// USB 设备轮询任务 (1ms 周期, FreeRTOS)
static void usb_task(void *pvParameters)
{
    (void)pvParameters;
    for (;;) {
        tusb_port_task();
        vendor_poll();
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

static StackType_t  lvgl_task_stack[4096];
static StaticTask_t lvgl_task_tcb;
// LVGL 任务: 初始化显示 + timer 心跳 (FreeRTOS)
static void lvgl_task(void *pvParameters)
{
  (void)pvParameters;
  lv_port_init();

  // 简单按钮示例
  lv_obj_t *btn = lv_button_create(lv_screen_active());
  lv_obj_set_size(btn, 120, 50);
  lv_obj_center(btn);

  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, "Hello");
  lv_obj_center(label);

  for (;;) {
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(5));
  };
}

static StackType_t  init_task_stack[1024];
static StaticTask_t init_task_tcb;\
static char buff[64];
// 初始化任务: RTOS资源/LCD测试/UART回传+时间戳 (FreeRTOS)
static void init_task(void *pvParameters)
{
    (void)pvParameters;
    tlsf_port_rtos_init();
    sd_rtos_init();
    uart_rtos_init();

    xTaskCreateStatic(led_task, "led", 128, NULL, 1,
                       led_task_stack, &led_task_tcb);
    xTaskCreateStatic(usb_task, "usb", 512, NULL, 2,
                       usb_task_stack, &usb_task_tcb);
    xTaskCreateStatic(lvgl_task, "lvgl", 4096, NULL, 4,
                       lvgl_task_stack, &lvgl_task_tcb);

    int uart = open("/dev/uart/COM0", O_RDWR);
    if (uart == -1) {
      printf("open failed\n");
      return;
    }

    while (1) {
      int s = read(uart, buff, sizeof(buff));
      if (s > 0) {
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[64];
        int n = strftime(time_str, sizeof(time_str),
                         "[%Y-%m-%d %H:%M:%S] ", tm_info);
        if (n > 0) {
          write(uart, time_str, n);
        }
        write(uart, buff, s);
      }
      vTaskDelay(pdMS_TO_TICKS(10));
    };
}

#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* Enable the CPU Cache */

  /* Enable I-Cache---------------------------------------------------------*/
  SCB_EnableICache();

  /* Enable D-Cache---------------------------------------------------------*/
  SCB_EnableDCache();

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
  MX_MDMA_Init();
  MX_DMA_Init();
  MX_QUADSPI_Init();
  MX_USB_OTG_FS_PCD_Init();
  MX_USART1_UART_Init();
  MX_FMC_Init();
  MX_SDMMC1_SD_Init();
  MX_RTC_Init();
  MX_LTDC_Init();
  MX_DMA2D_Init();
  /* USER CODE BEGIN 2 */
  set_rtc_from_build_time();
  qspi_w25q64_init();              // 初始化W25Q64
  qspi_w25q64_memory_mapped_mode(); // 启用内存映射，之后才能读 EX_FLASH
  FMC_SDRAM_CommandTypeDef command;	// 控制指令
  sdram_init_sequence(&hsdram1,&command);
  mem_itcm_load();       // 搬运 ITCM 函数
  mem_sdram_load();      // 搬运 SDRAM 函数 (从 EX_FLASH 读取)
  tlsf_port_init();
  sd_init();              // 初始化 SD 卡
  fatfs_port_init();      // 挂载 SD 卡文件系统 (f_mount)
  fatfs_port_vfs_init();  // 注册 VFS 节点 (根 "/sd")
  usb_msc_init();         // 缓存 SD 卡信息 (必须在 tusb_port_init 之前)
  vendor_init();          // 初始化 VENDOR 协议
  tusb_port_init();       // 初始化 TinyUSB 设备
  uart_init();                     // UART RXNE 中断在硬件初始化完成后才开启
  uart_vfs_init();
  lcd_init();

#if BSP_FREERTOS_ENABLED
  xTaskCreateStatic(init_task, "init", 1024, NULL, 3,
                     init_task_stack, &init_task_tcb);
  freertos_port_init();
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
#if !BSP_FREERTOS_ENABLED
  // lv_port_init();
  // lv_tick_set_cb(lv_port_tick_get);
  //
  // lv_obj_t *btn = lv_button_create(lv_screen_active());
  // lv_obj_set_size(btn, 120, 50);
  // lv_obj_center(btn);
  // lv_obj_t *label = lv_label_create(btn);
  // lv_label_set_text(label, "Hello");
  // lv_obj_center(label);
  lcd_set_back_color(LCD_WHITE);
  lcd_set_color(LCD_BLACK);
  lcd_clear();

  uint32_t t = HAL_GetTick();
  while (1)
  {
    // lv_timer_handler();
    extern uint8_t LCD_MEM_ADDRESS[];
    uint16_t *fb = (uint16_t *)LCD_MEM_ADDRESS;
    for (int y = 400; y < 440; y++) {
      for (int x = 680; x < 780; x++) {
        fb[y * 800 + x] = 0x0000;  // 黑色 RGB565
      }
    }
    if (HAL_GetTick()-t > 100) {
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      t = HAL_GetTick();
    }
    HAL_Delay(5);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
#endif
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

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 5;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 20;
  RCC_OscInitStruct.PLL.PLLR = 4;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

#if BSP_FREERTOS_ENABLED
static StackType_t  idle_task_stack[configMINIMAL_STACK_SIZE];
static StaticTask_t idle_task_tcb;

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
                                   StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    *ppxIdleTaskTCBBuffer   = &idle_task_tcb;
    *ppxIdleTaskStackBuffer = idle_task_stack;
    *pulIdleTaskStackSize   = configMINIMAL_STACK_SIZE;
}
#endif

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0xC0000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_32MB;
  MPU_InitStruct.SubRegionDisable = 0x0;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_FULL_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_ENABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_NOT_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Number = MPU_REGION_NUMBER1;
  MPU_InitStruct.BaseAddress = 0x30000000;
  MPU_InitStruct.Size = MPU_REGION_SIZE_256KB;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM7 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM7)
  {
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
#ifdef USE_FULL_ASSERT
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
