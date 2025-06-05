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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "fonts.h"
#include "ssd1306.h"
#include "stdio.h"
#include "BH1750.h"
#include "stm32f1xx_hal.h"
#include "string.h"
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
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
BH1750_device_t* light_sensor; //Con trỏ tới cấu trúc BH1750_device_t chứa thông tin về cảm biến BH1750
uint16_t Lux = 0;  // Biến toàn cục để hiển thị trong Live Expressions
uint32_t adcValue = 0;
float voltage = 0.0f;
float percentRH = 0.0f;
// Giá trị điện áp tham chiếu của cảm biến (hiệu chỉnh thực tế)
#define ADC_DRY   0
#define ADC_WET   2071
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM1_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define DHT22_PORT GPIOB
#define DHT22_PIN GPIO_PIN_9
uint8_t RH1, RH2, TC1, TC2, SUM, CHECK;
uint32_t pMillis, cMillis; //Biến dùng để đo thời gian (đơn vị ms) trong các vòng lặp chờ
float tCelsius = 0;
float tFahrenheit = 0;
float RH = 0;
uint8_t RHI, RHD, TCI, TCD, TFI, TFD; //Biến tách phần nguyên và phần thập phân của độ ẩm để hiển thị
char strCopy[32];

uint8_t rxByte;            // ➜ nhận từng byte
char cmdBuf[32];        // buffer lệnh FAN/PUMP
uint8_t cmdIdx = 0;
char currentMode[16] = "manual";  // Mặc định là manual

int __io_putchar(int ch) {
  HAL_UART_Transmit(&huart2, (uint8_t*)&ch, 1, HAL_MAX_DELAY);
  return ch;
}
void microDelay (uint16_t delay)
{
  __HAL_TIM_SET_COUNTER(&htim1, 0); //Reset bộ đếm của Timer TIM1 về 0
  while (__HAL_TIM_GET_COUNTER(&htim1) < delay); //Vòng lặp chờ cho đến khi giá trị bộ đếm của Timer đạt tới giá trị delay
}

uint8_t DHT22_Start (void)
{
  uint8_t Response = 0; //Khởi tạo biến để lưu phản hồi từ DHT22, mặc định là 0 (không có phản hồi)
  GPIO_InitTypeDef GPIO_InitStructPrivate = {0}; //Khai báo biến cấu trúc dùng để cấu hình chân GPIO cho DHT22 và khởi tạo các trường về 0
  //Cấu hình chân DHT22 (PB9) làm output kiểu đẩy (push-pull) với tốc độ thấp và không dùng nội bộ kéo lên hoặc kéo xuống (no pull)
  GPIO_InitStructPrivate.Pin = DHT22_PIN;
  GPIO_InitStructPrivate.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStructPrivate.Speed = GPIO_SPEED_FREQ_LOW;
  GPIO_InitStructPrivate.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStructPrivate); // set the pin as output
  HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, 0);   // pull the pin low
  microDelay (1300);   // wait for 1300us
  HAL_GPIO_WritePin (DHT22_PORT, DHT22_PIN, 1);   // pull the pin high
  microDelay (30);   // wait for 30us
  GPIO_InitStructPrivate.Mode = GPIO_MODE_INPUT;
  GPIO_InitStructPrivate.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(DHT22_PORT, &GPIO_InitStructPrivate); // set the pin as input
  microDelay (40);
  if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))) // Kiểm tra nếu chân đang ở mức LOW (phản hồi đầu tiên của DHT22)
  {
    microDelay (80);
    if ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN))) Response = 1;
  }
  pMillis = HAL_GetTick(); //sử dụng để đo thời gian chờ
  cMillis = HAL_GetTick();
  //chờ cho đến khi tín hiệu của DHT22 thay đổi (chuyển từ HIGH sang LOW) hoặc vượt quá một khoảng thời gian nhất định
  //(ở đây là 2 ms, mặc dù thời gian này chủ yếu để đảm bảo xung tín hiệu đã kết thúc)
  while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)) && pMillis + 2 > cMillis)
  {
    cMillis = HAL_GetTick();
  }
  return Response;
}

uint8_t DHT22_Read (void)
{
  uint8_t a,b;
  for (a=0;a<8;a++)
  {
    pMillis = HAL_GetTick(); //khởi tạo để đo thời gian chờ của từng bit
    cMillis = HAL_GetTick();
    while (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go high
      cMillis = HAL_GetTick(); //đảm bảo không bị treo nếu tín hiệu không thay đổi
    }
    microDelay (40);   // wait for 40 us
    if (!(HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)))   // if the pin is low
      b&= ~(1<<(7-a)); //set 0
    else
      b|= (1<<(7-a)); //set 1
    //Sau khi xác định giá trị của bit hiện tại, chương trình lại chờ cho đến khi tín hiệu chuyển về mức LOW, báo hiệu kết thúc truyền của bit đó
    pMillis = HAL_GetTick(); //lệnh này đo thời gian
    cMillis = HAL_GetTick();
    while ((HAL_GPIO_ReadPin (DHT22_PORT, DHT22_PIN)) && pMillis + 2 > cMillis)
    {  // wait for the pin to go low
      cMillis = HAL_GetTick();
    }
  }
  return b;
}
uint16_t Read_Soil_ADC(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 100);
    return HAL_ADC_GetValue(&hadc1);
}

uint8_t ADC_To_Moist(uint16_t adc)
{
    if (adc < ADC_DRY) adc = ADC_DRY;
    if (adc > ADC_WET) adc = ADC_WET;
    return (adc - ADC_DRY) * 100 / (ADC_WET - ADC_DRY);
}
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
  MX_I2C2_Init();
  MX_TIM1_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim1); //Bắt đầu Timer TIM1 để có thể sử dụng trong việc tạo độ trễ
  SSD1306_Init(); //Khởi tạo màn hình OLED SSD1306 qua giao tiếp I2C (bao gồm việc xóa màn hình, thiết lập các thông số ban đầu)
  /* Khởi tạo BH1750 */
  //Hàm này cấp phát và khởi tạo cấu trúc cho cảm biến BH1750: con trỏ tới I2C, tên gọi BH1750 nhận diện, true: thông báo chân ADDR nối với GND
  light_sensor = BH1750_init_dev_struct(&hi2c1, "BH1750", true);
  BH1750_init_dev(light_sensor); //Gửi các lệnh khởi tạo cần thiết (Power On, Reset, chọn chế độ đo) tới cảm biến BH1750
  HAL_ADC_Start(&hadc1); //ADC bat dau khoi dong
  HAL_UART_Receive_IT(&huart2, &rxByte, 1);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  if(DHT22_Start())
	      {
	        RH1 = DHT22_Read(); // First 8bits of humidity
	        RH2 = DHT22_Read(); // Second 8bits of Relative humidity
	        TC1 = DHT22_Read(); // First 8bits of Celsius
	        TC2 = DHT22_Read(); // Second 8bits of Celsius
	        SUM = DHT22_Read(); // Check sum
	        CHECK = RH1 + RH2 + TC1 + TC2;
	        if (CHECK == SUM)
	        {
	          if (TC1>127) // If TC1=10000000, negative temperature
	          {
	            tCelsius = (float)TC2/10*(-1);
	          }
	          else
	          {
	            tCelsius = (float)((TC1<<8)|TC2)/10;
	          }
	          RH = (float) ((RH1<<8)|RH2)/10;
	          SSD1306_GotoXY (0, 0); //Đưa con trỏ vẽ trên màn hình OLED về vị trí (0,0) (góc trên bên trái) để bắt đầu hiển thị kết quả độ ẩm
	          RHI = RH;  // Relative humidity integral
	          RHD = RH*10-RHI*10; // Relative humidity decimal
	          int isNegative = 0;
	          if (tCelsius < 0)
	          {
	            TCI = tCelsius *(-1);  // Celsius integral
	            TCD = tCelsius*(-10)-TCI*10; // Celsius decimal
	            isNegative = 1;
	          }
	          else
	          {
	            TCI = tCelsius;  // Celsius integral
	            TCD = tCelsius*10-TCI*10; // Celsius decimal
	          }
	          SSD1306_GotoXY (0, 0);
	          if (isNegative)
	              sprintf(strCopy, "H:%d.%d%% T:-%d.%dC", RHI, RHD, TCI, TCD);
	          else
	              sprintf(strCopy, "H:%d.%d%% T:%d.%dC", RHI, RHD, TCI, TCD);
	          SSD1306_Puts(strCopy, &Font_7x10, 1);
	        }
	      }
	      //HAL_Delay(1000); //ừng chương trình 1000 ms (1 giây) trước khi bắt đầu vòng lặp tiếp theo, đảm bảo tần suất đọc dữ liệu 1 giây một lần
	   /* Đọc dữ liệu từ cảm biến BH1750 */
	   light_sensor->poll(light_sensor); //đọc dữ liệu và chuyển đổi thành giá trị lux
	   Lux = light_sensor->value;  // Cập nhật biến Lux
	   SSD1306_GotoXY(0, 20);
	   SSD1306_Puts("              ", &Font_7x10, 1);  // Xóa dòng cũ
	   SSD1306_GotoXY(0, 20);
	   sprintf(strCopy, "Lux: %d", Lux);
	   SSD1306_Puts(strCopy, &Font_7x10, 1);
	  //HAL_Delay(1000); // Đọc mỗi giây
	  //ADC
	  adcValue = Read_Soil_ADC();
	  percentRH = ADC_To_Moist(adcValue);
	  	        //HAL_Delay(1000); //Chờ 1 giây trước khi đọc ADC tiếp theo
	  SSD1306_GotoXY(0, 40);
	  sprintf(strCopy, "Soil: %.1f%%", percentRH);
	  SSD1306_Puts(strCopy, &Font_7x10, 1);
	  SSD1306_UpdateScreen();
	  // ==== CẢNH BÁO QUA BUZZER ====
	  if (tCelsius > 30 || RH < 70 || percentRH < 70 || Lux < 100 || Lux > 1000)
	  {
	      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1); // Buzzer kêu
	      HAL_Delay(500);
	      HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_1); // Tắt buzzer
	  } else {
	      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); // Tắt buzzer nếu điều kiện OK
	  }
	  char uartBuffer[100];
	  snprintf(uartBuffer, sizeof(uartBuffer),
	           "{\"temperature\":%.1f,\"humidity\":%.1f,\"soil_moisture\":%.1f,\"light\":%d}\n",
	           tCelsius, RH, percentRH, Lux);

	  HAL_UART_Transmit(&huart2, (uint8_t*)uartBuffer, strlen(uartBuffer), 100);
	  HAL_Delay(1000); // gửi mỗi giây

    /* USER CODE END WHILE */
	  if (strcmp(currentMode, "auto") == 0)
	  {
	      if (tCelsius > 30)
	          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // FAN ON
	      else
	          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);   // FAN OFF

	      if (percentRH < 60.0f || RH < 50)
	          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // PUMP ON
	      else if (percentRH > 70.0f || RH > 70)
	          HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   // PUMP OFF
	  }
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == USART2)
  {
    if (rxByte == '\n' || rxByte == '\r')
    {
      cmdBuf[cmdIdx] = '\0';

      if (strstr(cmdBuf, "MODE:auto"))      strcpy(currentMode, "auto");
      else if (strstr(cmdBuf, "MODE:manual")) strcpy(currentMode, "manual");

      if (strcmp(currentMode, "manual") == 0) {
    	  if      (strstr(cmdBuf, "FAN:on"))   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET);
    	  else if (strstr(cmdBuf, "FAN:off"))  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);

    	  if      (strstr(cmdBuf, "PUMP:on"))  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET);
    	  else if (strstr(cmdBuf, "PUMP:off")) HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);
      }
      cmdIdx = 0;
    }
    else if (cmdIdx < sizeof(cmdBuf) - 1)
    {
      cmdBuf[cmdIdx++] = rxByte;
    }

    HAL_UART_Receive_IT(&huart2, &rxByte, 1);
  }
}
/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 400000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

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

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 71;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
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
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_9, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB12 PB13 PB9 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
