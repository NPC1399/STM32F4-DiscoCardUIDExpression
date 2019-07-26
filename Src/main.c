/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_host.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32f4xx_hal.h"
#include "stm32f429i_discovery_lcd.h"
#include "ring_buff.h"
#include "stdbool.h"
#include "at_esp32.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define LCD_FRAME_BUFFER_LAYER0		(LCD_FRAME_BUFFER+0x130000)
#define LCD_FRAME_BUFFER_LAYER1		LCD_FRAME_BUFFER
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CRC_HandleTypeDef hcrc;

DMA2D_HandleTypeDef hdma2d;

I2C_HandleTypeDef hi2c3;

LTDC_HandleTypeDef hltdc;

SPI_HandleTypeDef hspi5;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim7;

UART_HandleTypeDef huart5;
UART_HandleTypeDef huart1;

SDRAM_HandleTypeDef hsdram1;

/* USER CODE BEGIN PV */
volatile enum State state = ESP_OFFLINE; // initial state
Ring_buff_t buff;             // ring buffer
uint8_t     buffSize = 100;   // defines ring buffer's size
uint8_t     receivedData[1];  // used to store a char received from uart
char        linBuff[100];     // linear buffer used to process a command
int         pos = 0;          // represents position in a linear buffer
uint8_t     tmpBuffer[32];    // used for storing card data such as UID or PUPI
uint8_t     tmpBuffer0[32];   // used for storing data from tmpBuffer if it can't send into mqtt server
uint8_t     lcdDataBuff[32];  // used for storing data to display to LCD(LCM1602)
int TypeReaderFlag = 0;

// flags
bool espAlive       = false;  // tells whether esp32 is alive
bool commandSent    = false;  // used to prevent sending command repetitively
bool mqttLogin      = false;
bool publishReq     = false;
bool subscribeReq   = true;
bool ForceClose     = false;
bool alreadyConnect = false;
bool unsuccess      = false;
int Time_Out = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_DMA2D_Init(void);
static void MX_FMC_Init(void);
static void MX_I2C3_Init(void);
static void MX_LTDC_Init(void);
static void MX_SPI5_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM7_Init(void);
static void MX_UART5_Init(void);
static void MX_USART1_UART_Init(void);
void MX_USB_HOST_Process(void);

/* USER CODE BEGIN PFP */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void resetLinBuff(char* linBuff, int size);
void delay(int millis);
void sanitizeLinBuff(char* linBuff, int size);
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
  MX_CRC_Init();
  MX_DMA2D_Init();
  MX_FMC_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_SPI5_Init();
  MX_TIM1_Init();
  MX_TIM7_Init();
  MX_UART5_Init();
  MX_USART1_UART_Init();
  MX_USB_HOST_Init();
  /* USER CODE BEGIN 2 */
  /* Initialize the LCD */
  BSP_LCD_Init();
  BSP_LCD_LayerDefaultInit(1,LCD_FRAME_BUFFER_LAYER1);
  BSP_LCD_SelectLayer(1);
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetColorKeying(1, LCD_COLOR_WHITE);
  BSP_LCD_SetLayerVisible(1, DISABLE);
  BSP_LCD_LayerDefaultInit(0, LCD_FRAME_BUFFER_LAYER0);
  BSP_LCD_SelectLayer(0);
  BSP_LCD_DisplayOn();
  BSP_LCD_Clear(LCD_COLOR_WHITE);
  BSP_LCD_SetFont(&Font12);
  /***********************/
  /* Initialize ring buffer */
  uint8_t* buffPtr = (uint8_t*) malloc(buffSize);
  initRingBuff(&buff, buffSize, buffPtr);
  /**************************/
  HAL_UART_Receive_IT(&huart5, receivedData, 1);  //Enable interrupt
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  BSP_LCD_SetTextColor(LCD_COLOR_DARKBLUE);
  BSP_LCD_DisplayStringAt(0, 10, (uint8_t*)"STM32F429I BSP", CENTER_MODE);
  while (1)
  {
    /* USER CODE END WHILE */
    MX_USB_HOST_Process();

    /* USER CODE BEGIN 3 */
    /* ----- universal response checker ----- */
    // most response in this section appears unpredictably at any time of connection
    // so they must be placed outside every state

    // check for wifi connection
    if (strstr(linBuff, "WIFI GOT IP")) {
      HAL_UART_Transmit(&huart1, (uint8_t*) "[i] Wi-Fi connected.\r\n", sizeof("[i] Wi-Fi connected.\r\n") - 1, 100);
      
      // entering wifi connected state
      HAL_UART_Transmit(&huart1, (uint8_t*) "*--- IDLE MODE waiting for command ---*\r\n", sizeof("*--- IDLE MODE waiting for command ---*\r\n") - 1, 100);

      // flags
      espAlive = true;
      commandSent = false;

      // state control
      state = WIFI_CONNECTED;

      // prompt to LCD
      HAL_Delay(100);
      BSP_LCD_Clear(LCD_COLOR_WHITE);
      HAL_Delay(100);
      BSP_LCD_DisplayStringAt(0,10,(uint8_t*)"Wi-Fi Connected", CENTER_MODE);
      resetLinBuff(linBuff, 100);
    }

    // check for disconnection
    else if (strstr(linBuff, "DISCONNECT")) {
      HAL_UART_Transmit(&huart1, (uint8_t*) "[i] Wi-Fi is disconnected.\r\n", sizeof("[i] Wi-Fi is disconnected.\r\n") - 1, 100);

      // flags
      commandSent = false;

      // state control
      state = ESP_ALIVE;

      // prompt to LCD
      HAL_Delay(100);
      BSP_LCD_Clear(LCD_COLOR_WHITE);
      HAL_Delay(100);
      BSP_LCD_DisplayStringAt(0,10,(uint8_t*)"Wi-Fi Disconnected", CENTER_MODE);
      resetLinBuff(linBuff, 100);
    }

    // check for TCP disconnection
    else if (strstr(linBuff, "CLOSED")) {
      if(!ForceClose){
        HAL_UART_Transmit(&huart1, (uint8_t*) "[i] TCP connection closed.\r\n", sizeof("[i] TCP connection closed.\r\n") - 1, 100);

        //flags
        commandSent = false;
        //tcpEstablished = false;
        mqttLogin = false;
        subscribeReq = true; //uncomment this for using subscriber mode

        // state control
        state = QUIT_AP;

        resetLinBuff(linBuff, 100);

        // turn of onboard LED
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

        // prompt to LCD
        HAL_Delay(100);
        BSP_LCD_Clear(LCD_COLOR_WHITE);
        HAL_Delay(100);
        BSP_LCD_DisplayStringAt(0,10,(uint8_t*)"TCP connection Closed", CENTER_MODE);
      }
    }


    /* ---------- FLOW CONTROL ---------- */
    
    switch (state)
    {
      case ESP_OFFLINE:
        if (strstr(linBuff, "ready")) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[i] ESP is alive\r\n", sizeof("[i] ESP is alive\r\n") - 1, 100);

          // flags
          commandSent = false;
          espAlive = true;    
          // state control
          state = ESP_ALIVE;

          resetLinBuff(linBuff, 100);
        }

        if (!commandSent && !espAlive) {
          resetEsp();

          // flags
          commandSent = true;
        }
        break;
        
      case ESP_ALIVE:

        // ERROR Handling
        if (strstr(linBuff, "+CWJAP:1")) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[!] err: Connection Timeout\r\n", sizeof("[!] err: Connection Timeout\r\n") - 1, 100);
          commandSent = false;
          resetLinBuff(linBuff, 100);
        }
        else if (strstr(linBuff, "+CWJAP:2")) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[!] err: Wrong Password\r\n", sizeof("[!] err: Wrong Password\r\n") - 1, 100);
          commandSent = false;
          resetLinBuff(linBuff, 100);
        }
        else if (strstr(linBuff, "+CWJAP:3")) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[!] err: Target AP not found\r\n", sizeof("[!] err: Target AP not found\r\n") - 1, 100);
          commandSent = false;
          resetLinBuff(linBuff, 100);
        }
        else if (strstr(linBuff, "+CWJAP:4")) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[!] err: Connection Failed\r\n", sizeof("[!] err: Connection Failed\r\n") - 1, 100);
          commandSent = false;
          resetLinBuff(linBuff, 100);
        }
        
        // send command
        if (!commandSent) {
          connectToWifi("testespap", "phumiphat123");
          
          // flags
          commandSent = true;
        }

        break;

      case WIFI_CONNECTED: // This is the IDLE state of Operation

          if (strstr(linBuff, "CONNECT")) {
            HAL_UART_Transmit(&huart1, (uint8_t*) "[i] TCP connection established.\r\n", sizeof("[i] TCP connection established.\r\n") - 1, 100);

            // flags
            commandSent = false;
            espAlive = true;
          
            // state control
            state = TCP_CONNECTED;

            // prompt to LCD
            HAL_Delay(100);
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            HAL_Delay(100);
            BSP_LCD_DisplayStringAt(0,0,(uint8_t*)"TCP connected",LEFT_MODE);
            resetLinBuff(linBuff, 100);
          }

        if (!commandSent && state == WIFI_CONNECTED && (publishReq || subscribeReq ||unsuccess)) {
          HAL_Delay(1000);
          connectToTCP("postman.cloudmqtt.com", "16001", "7200");
          commandSent = true;
        }
        break;


      case TCP_CONNECTED: ;       
        // check if mqtt is login
        char mqttSuccessLoginMsg[] = {0x20, 0x02, 0x00, 0x00};
        // Waiting for new command after send success
        if(mqttLogin){
          if(alreadyConnect == false){
            HAL_UART_Transmit(&huart1,(uint8_t*)"Waiting for command.\r\n",sizeof("Waiting for command\r\n"),100);
          }
          alreadyConnect = true;
          if (subscribeReq) {
            alreadyConnect = false;
            state = SUBSCRIBER;
            HAL_UART_Transmit(&huart1, (uint8_t*) "[i] SUBSCRIBER state entered.\r\n", sizeof("[i] SUBSCRIBER state entered.\r\n") - 1, 100);
          }
        }
        if (!mqttLogin && strstr(linBuff, mqttSuccessLoginMsg)) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[i] Successfully login to mqtt server.\r\n", sizeof("[i] Successfully login to mqtt server.\r\n") - 1, 100);
          
          // flags
          commandSent = false;
          mqttLogin = true;

          // state control
          if (subscribeReq) {
            state = SUBSCRIBER;
            HAL_UART_Transmit(&huart1, (uint8_t*) "[i] SUBSCRIBER state entered.\r\n", sizeof("[i] SUBSCRIBER state entered.\r\n") - 1, 100);
          }
          //resending data
          else if(unsuccess){
            state = PUBLISHER;
          }
          else {
            state = PUBLISHER;
            HAL_UART_Transmit(&huart1, (uint8_t*) "[i] PUBLISHER state entered.\r\n", sizeof("[i] PUBLISHER state entered.\r\n") - 1, 100);
          }

          pos = 0;
          resetLinBuff(linBuff, 100);
        }

        // send command
        if (!commandSent && !mqttLogin) {
          // send login command
          char mqttLogin[] = { 0x10, 0x27, 0x00,0x04, 0x4D, 0x51, 0x54, 0x54, 0x04, 0xC2, 0x00, 0x3C, 0x00, 0x03,
                              0x43, 0x4F, 0x50, 0x00, 0x08, 0x7A, 0x6C, 0x64, 0x62, 0x67, 0x6F, 0x79, 0x6D, 0x00,
                              0x0C, 0x55, 0x6C, 0x6D, 0x58, 0x52, 0x46, 0x33, 0x42, 0x48, 0x39, 0x65, 0x4F};
          tcpSendDataRequest("41");
          tcpSendData(mqttLogin, 41);
          commandSent = true;
        }
      
        break;

      case QUIT_AP:
        if (strstr(linBuff, "OK")) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[i] Successfully disconnected from an AP.\r\n", sizeof("[i] Successfully disconnected from an AP.\r\n") - 1, 100);
          
          // flags
          commandSent = false;

          // state control
          state = ESP_ALIVE;
          
          resetLinBuff(linBuff, 100);
        }

        if(!commandSent) {
          disconnectWifi();

          // flags
          commandSent = true;
        }

        break;

      case SUBSCRIBER: ;
        // check if subscribe is complete
        char mqttSuccessSubscribe[] = { 0x90, 0x03, 0x00, 0x01, 0x01 };
        if (subscribeReq && mqttLogin && strstr(linBuff, mqttSuccessSubscribe)) {
          HAL_UART_Transmit(&huart1, (uint8_t*) "[i] Successfully subscribe to a message.\r\n", sizeof("[i] Successfully subscribe to a message.\r\n") - 1, 100);

          // flags
          subscribeReq = false;
          commandSent = false;

          // state control
          state = SUBSCRIBER;

          // turn on onboard LED
          HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

          // prompt to LCD
          HAL_Delay(100);
          BSP_LCD_Clear(LCD_COLOR_WHITE);
          HAL_Delay(100);
          BSP_LCD_DisplayStringAt(0,20,(uint8_t*)"Subcribed to a",LEFT_MODE);
          BSP_LCD_DisplayStringAt(0,40,(uint8_t*)"Message",LEFT_MODE);
          pos = 0;
          resetLinBuff(linBuff, 100);
        }

        if (!subscribeReq && strstr(linBuff, "+IPD,")) {
          HAL_Delay(100); // delay to wait for data to be loaded into linBuff
          
          // sanitize input to eliminate '\0'
          sanitizeLinBuff(linBuff, 100);
          
          // get pointer of substring
          char *ptrTopic = strstr(linBuff, "foo");

          // locate substring position
          int position = ptrTopic - linBuff;
        
          if (ptrTopic) {
            for (int i = position + 3; i < 100; ++i) {
              HAL_UART_Transmit(&huart1, (uint8_t*) &linBuff[i], 1, 100);

              if (!(linBuff[i] == ' ')) {
                lcdDataBuff[i-position-3] = linBuff[i]; // add UID to lcd data buffer
              }
            }
          }
          HAL_UART_Transmit(&huart1, (uint8_t*) "\r\n", 2, 100);
          
          // clear lcd before display any data
         BSP_LCD_Clear(LCD_COLOR_WHITE);
          
          // display on LCD
          if (lcdDataBuff[0] == '1') {
            // write card type
            HAL_Delay(100);
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            HAL_Delay(100);
            BSP_LCD_DisplayStringAt(0,20,(uint8_t*)"ISO14443A",LEFT_MODE);

            // write card UID
            BSP_LCD_DisplayStringAt(0,40,lcdDataBuff,LEFT_MODE);
          }
          else if (lcdDataBuff[0] == '2') {
            // write card type
            HAL_Delay(100);
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            HAL_Delay(100);
            BSP_LCD_DisplayStringAt(0,20,(uint8_t*)"ISO14443B",LEFT_MODE);

            // write card UID
            BSP_LCD_DisplayStringAt(0,40,lcdDataBuff,LEFT_MODE);
          }
          else if (lcdDataBuff[0] == '3') {
            // write card type
            HAL_Delay(100);
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            HAL_Delay(100);
            BSP_LCD_DisplayStringAt(0,20,(uint8_t*)"ISO15693",LEFT_MODE);

            // write card UID
            BSP_LCD_DisplayStringAt(0,40,lcdDataBuff,LEFT_MODE);
          }
          else {
            // write no card detected
            HAL_Delay(100);
            BSP_LCD_Clear(LCD_COLOR_WHITE);
            HAL_Delay(100);
            BSP_LCD_DisplayStringAt(0,20,(uint8_t*)"No Card",LEFT_MODE);
            BSP_LCD_DisplayStringAt(0,40,(uint8_t*)"Detected",LEFT_MODE);

          }

          pos = 0;
          resetLinBuff(linBuff, 100);
          //add null to lcdDataBuff to reset
          for(int i = 0;i<32;i++){
            lcdDataBuff[i] = '\0';
          }
        }
        
        if (!commandSent && mqttLogin && subscribeReq) {
          // subscribe to a message
          char subFoo[10] = { 0x82, 0x08, 0x00, 0x01, 0x00, 0x03, 0x66, 0x6F, 0x6F, 0x01 };
          tcpSendDataRequest("10");
          tcpSendData(subFoo, 10);

          // flags
          commandSent = true;

        }

        break;
      case QUIT_TCP:
        //Disconnect TCP server 
        if (strstr(linBuff,"OK") && commandSent){
          HAL_UART_Transmit(&huart1,(uint8_t*)"[i] TCP was closed.\r\n",sizeof("[i] TCP was closed.\r\n")-1,100);
          ForceClose = false;
          publishReq = false;
          mqttLogin = false;
          commandSent = false;
          state = WIFI_CONNECTED; //return to WIFI_CONNECTED state 
          //reset buffer
          pos = 0;
          resetLinBuff(linBuff, 100);
        }
        if(ForceClose && !commandSent){ 
          closeTCP();
          commandSent = true;
        }
        break;
      case TEST_STATE:
        // test case goes here
    	  HAL_Delay(100);
        BSP_LCD_Clear(LCD_COLOR_WHITE);
        HAL_Delay(100);
        BSP_LCD_DisplayStringAt(0,0,(uint8_t*)"Subcribed to a",LEFT_MODE);
        BSP_LCD_DisplayStringAt(0,20,(uint8_t*)"Message",LEFT_MODE);
        while(1);


        break;

      default:
        break;
    }


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
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Configure the main internal regulator output voltage 
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
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
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 50;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 2;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief CRC Initialization Function
  * @param None
  * @retval None
  */
static void MX_CRC_Init(void)
{

  /* USER CODE BEGIN CRC_Init 0 */

  /* USER CODE END CRC_Init 0 */

  /* USER CODE BEGIN CRC_Init 1 */

  /* USER CODE END CRC_Init 1 */
  hcrc.Instance = CRC;
  if (HAL_CRC_Init(&hcrc) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CRC_Init 2 */

  /* USER CODE END CRC_Init 2 */

}

/**
  * @brief DMA2D Initialization Function
  * @param None
  * @retval None
  */
static void MX_DMA2D_Init(void)
{

  /* USER CODE BEGIN DMA2D_Init 0 */

  /* USER CODE END DMA2D_Init 0 */

  /* USER CODE BEGIN DMA2D_Init 1 */

  /* USER CODE END DMA2D_Init 1 */
  hdma2d.Instance = DMA2D;
  hdma2d.Init.Mode = DMA2D_M2M;
  hdma2d.Init.ColorMode = DMA2D_OUTPUT_ARGB8888;
  hdma2d.Init.OutputOffset = 0;
  hdma2d.LayerCfg[1].InputOffset = 0;
  hdma2d.LayerCfg[1].InputColorMode = DMA2D_INPUT_ARGB8888;
  hdma2d.LayerCfg[1].AlphaMode = DMA2D_NO_MODIF_ALPHA;
  hdma2d.LayerCfg[1].InputAlpha = 0;
  if (HAL_DMA2D_Init(&hdma2d) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_DMA2D_ConfigLayer(&hdma2d, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN DMA2D_Init 2 */

  /* USER CODE END DMA2D_Init 2 */

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
  /** Configure Analogue filter 
  */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c3, I2C_ANALOGFILTER_ENABLE) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Digital filter 
  */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c3, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C3_Init 2 */

  /* USER CODE END I2C3_Init 2 */

}

/**
  * @brief LTDC Initialization Function
  * @param None
  * @retval None
  */
static void MX_LTDC_Init(void)
{

  /* USER CODE BEGIN LTDC_Init 0 */

  /* USER CODE END LTDC_Init 0 */

  LTDC_LayerCfgTypeDef pLayerCfg = {0};

  /* USER CODE BEGIN LTDC_Init 1 */

  /* USER CODE END LTDC_Init 1 */
  hltdc.Instance = LTDC;
  hltdc.Init.HSPolarity = LTDC_HSPOLARITY_AL;
  hltdc.Init.VSPolarity = LTDC_VSPOLARITY_AL;
  hltdc.Init.DEPolarity = LTDC_DEPOLARITY_AL;
  hltdc.Init.PCPolarity = LTDC_PCPOLARITY_IPC;
  hltdc.Init.HorizontalSync = 9;
  hltdc.Init.VerticalSync = 1;
  hltdc.Init.AccumulatedHBP = 29;
  hltdc.Init.AccumulatedVBP = 3;
  hltdc.Init.AccumulatedActiveW = 269;
  hltdc.Init.AccumulatedActiveH = 323;
  hltdc.Init.TotalWidth = 279;
  hltdc.Init.TotalHeigh = 327;
  hltdc.Init.Backcolor.Blue = 0;
  hltdc.Init.Backcolor.Green = 0;
  hltdc.Init.Backcolor.Red = 0;
  if (HAL_LTDC_Init(&hltdc) != HAL_OK)
  {
    Error_Handler();
  }
  pLayerCfg.WindowX0 = 0;
  pLayerCfg.WindowX1 = 240;
  pLayerCfg.WindowY0 = 0;
  pLayerCfg.WindowY1 = 320;
  pLayerCfg.PixelFormat = LTDC_PIXEL_FORMAT_RGB565;
  pLayerCfg.Alpha = 255;
  pLayerCfg.Alpha0 = 0;
  pLayerCfg.BlendingFactor1 = LTDC_BLENDING_FACTOR1_PAxCA;
  pLayerCfg.BlendingFactor2 = LTDC_BLENDING_FACTOR2_PAxCA;
  pLayerCfg.FBStartAdress = 0xD0000000;
  pLayerCfg.ImageWidth = 240;
  pLayerCfg.ImageHeight = 320;
  pLayerCfg.Backcolor.Blue = 0;
  pLayerCfg.Backcolor.Green = 0;
  pLayerCfg.Backcolor.Red = 0;
  if (HAL_LTDC_ConfigLayer(&hltdc, &pLayerCfg, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LTDC_Init 2 */

  /* USER CODE END LTDC_Init 2 */

}

/**
  * @brief SPI5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI5_Init(void)
{

  /* USER CODE BEGIN SPI5_Init 0 */

  /* USER CODE END SPI5_Init 0 */

  /* USER CODE BEGIN SPI5_Init 1 */

  /* USER CODE END SPI5_Init 1 */
  /* SPI5 parameter configuration*/
  hspi5.Instance = SPI5;
  hspi5.Init.Mode = SPI_MODE_MASTER;
  hspi5.Init.Direction = SPI_DIRECTION_2LINES;
  hspi5.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi5.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi5.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi5.Init.NSS = SPI_NSS_SOFT;
  hspi5.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi5.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi5.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi5.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi5.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI5_Init 2 */

  /* USER CODE END SPI5_Init 2 */

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
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 0;
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
  * @brief TIM7 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM7_Init(void)
{

  /* USER CODE BEGIN TIM7_Init 0 */

  /* USER CODE END TIM7_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM7_Init 1 */

  /* USER CODE END TIM7_Init 1 */
  htim7.Instance = TIM7;
  htim7.Init.Prescaler = 374;
  htim7.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim7.Init.Period = 127;
  htim7.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim7) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim7, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM7_Init 2 */

  /* USER CODE END TIM7_Init 2 */

}

/**
  * @brief UART5 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART5_Init(void)
{

  /* USER CODE BEGIN UART5_Init 0 */

  /* USER CODE END UART5_Init 0 */

  /* USER CODE BEGIN UART5_Init 1 */

  /* USER CODE END UART5_Init 1 */
  huart5.Instance = UART5;
  huart5.Init.BaudRate = 115200;
  huart5.Init.WordLength = UART_WORDLENGTH_8B;
  huart5.Init.StopBits = UART_STOPBITS_1;
  huart5.Init.Parity = UART_PARITY_NONE;
  huart5.Init.Mode = UART_MODE_TX_RX;
  huart5.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart5.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart5) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART5_Init 2 */

  /* USER CODE END UART5_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/* FMC initialization function */
static void MX_FMC_Init(void)
{

  /* USER CODE BEGIN FMC_Init 0 */

  /* USER CODE END FMC_Init 0 */

  FMC_SDRAM_TimingTypeDef SdramTiming = {0};

  /* USER CODE BEGIN FMC_Init 1 */

  /* USER CODE END FMC_Init 1 */

  /** Perform the SDRAM1 memory initialization sequence
  */
  hsdram1.Instance = FMC_SDRAM_DEVICE;
  /* hsdram1.Init */
  hsdram1.Init.SDBank = FMC_SDRAM_BANK2;
  hsdram1.Init.ColumnBitsNumber = FMC_SDRAM_COLUMN_BITS_NUM_8;
  hsdram1.Init.RowBitsNumber = FMC_SDRAM_ROW_BITS_NUM_12;
  hsdram1.Init.MemoryDataWidth = FMC_SDRAM_MEM_BUS_WIDTH_16;
  hsdram1.Init.InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4;
  hsdram1.Init.CASLatency = FMC_SDRAM_CAS_LATENCY_3;
  hsdram1.Init.WriteProtection = FMC_SDRAM_WRITE_PROTECTION_DISABLE;
  hsdram1.Init.SDClockPeriod = FMC_SDRAM_CLOCK_PERIOD_2;
  hsdram1.Init.ReadBurst = FMC_SDRAM_RBURST_DISABLE;
  hsdram1.Init.ReadPipeDelay = FMC_SDRAM_RPIPE_DELAY_1;
  /* SdramTiming */
  SdramTiming.LoadToActiveDelay = 2;
  SdramTiming.ExitSelfRefreshDelay = 7;
  SdramTiming.SelfRefreshTime = 4;
  SdramTiming.RowCycleDelay = 7;
  SdramTiming.WriteRecoveryTime = 3;
  SdramTiming.RPDelay = 2;
  SdramTiming.RCDDelay = 2;

  if (HAL_SDRAM_Init(&hsdram1, &SdramTiming) != HAL_OK)
  {
    Error_Handler( );
  }

  /* USER CODE BEGIN FMC_Init 2 */

  /* USER CODE END FMC_Init 2 */
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(ACP_RST_GPIO_Port, ACP_RST_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, RDX_Pin|WRX_DCX_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOG, LD3_Pin|LD4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : NCS_MEMS_SPI_Pin CSX_Pin OTG_FS_PSO_Pin */
  GPIO_InitStruct.Pin = NCS_MEMS_SPI_Pin|CSX_Pin|OTG_FS_PSO_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : B1_Pin MEMS_INT1_Pin MEMS_INT2_Pin TP_INT1_Pin */
  GPIO_InitStruct.Pin = B1_Pin|MEMS_INT1_Pin|MEMS_INT2_Pin|TP_INT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : ACP_RST_Pin */
  GPIO_InitStruct.Pin = ACP_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(ACP_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : OTG_FS_OC_Pin */
  GPIO_InitStruct.Pin = OTG_FS_OC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_EVT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(OTG_FS_OC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : BOOT1_Pin */
  GPIO_InitStruct.Pin = BOOT1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(BOOT1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : TE_Pin */
  GPIO_InitStruct.Pin = TE_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(TE_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RDX_Pin WRX_DCX_Pin */
  GPIO_InitStruct.Pin = RDX_Pin|WRX_DCX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pins : LD3_Pin LD4_Pin */
  GPIO_InitStruct.Pin = LD3_Pin|LD4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) 
{
  // add a char into  ring buffer
  addToBuff(&buff, receivedData[0]);
  
  // reset linear buffer position every time encounters '\r'
  if (receivedData[0] == '\r') {
    pos = 0;
  }

  // add data to linear buffer everything it reads from ring buffer
  linBuff[pos] = *readBuff(&buff);
  pos++;

  // recall uart receive IT
  HAL_UART_Receive_IT(&huart5, receivedData, 1);
}

void delay(int millis)
{
  /*
    This function is user defined delay function using TIM6.
   */
  int i = 0;
  while ( i < millis ) {
    if (TIM7->CNT == 127) {
    	TIM7->CNT = 0;
      ++i;
    }
  }
}


void resetLinBuff(char* linBuff, int size)
{
  /*
    This function reset a linear buffer by inserting null char into all positions
   */
  for (int i = 0; i < size; ++i) {
    linBuff[i] = '\0';
  }
}

void sanitizeLinBuff(char* linBuff, int size)
{
  /*
    This function sanitizes the linear buffer for string processing by replace all '\0' with a 0x20(spacebar)
   */
  for (int i = 0; i < size; ++i) {
    if (linBuff[i] == 0) {
      linBuff[i] = 0x20;
    }
  }
}
/* USER CODE END 4 */

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
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
