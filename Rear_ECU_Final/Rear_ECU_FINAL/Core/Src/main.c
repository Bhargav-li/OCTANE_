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
#include <math.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
//===============I2C=========
#define MPU6050_ADDR   (0x68 << 1)  // HAL wants 8-bit address
#define PWR_MGMT_1     0x6B
#define SMPLRT_DIV     0x19
#define CONFIG_REG     0x1A
#define GYRO_CONFIG    0x1B
#define ACCEL_CONFIG   0x1C
#define WHO_AM_I       0x75
#define ACCEL_XOUT_H   0x3B

#define MLX90614_ADDR1   (0x5A << 1)  // HAL wants 8-bit address
#define MLX90614_ADDR2   (0x5B << 1)
#define MLX90614_TA      0x06         // ambient temp
#define MLX90614_TOBJ1   0x07
//===============I2C=========

//================Thermals=============
#define ADC_BUFF_SIZE 5
#define ADC_BRAKE_INDEX 0

#define Brake_pedal_threshold 150

#define ADC_THERM1_INDEX 1
#define ADC_THERM2_INDEX 2
#define ADC_THERM3_INDEX 3
#define ADC_THERM4_INDEX 4
#define ADC_MAX_VAL 4095      // 12-bit ADC resolution
#define TEMP_MAX_VAL 100.0f      // Max expected stator temp in degrees
#define MDOT_MAX_VAL 0.267f      // Max mass flow rate in kg/s
#define PWM_PERIOD 59999       // PWM Period
#define Table_Size 10
//===============Thermals============
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CAN_HandleTypeDef hcan;

I2C_HandleTypeDef hi2c1;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
//===============================I2C=================
int16_t mpu_buf[6];
float temp_ambient1, temp_obj1;
float temp_ambient2, temp_obj2;
//===============================I2C=================

//=============CAN==================


CAN_TxHeaderTypeDef TxHeader;
CAN_RxHeaderTypeDef RxHeader;
uint8_t TxData[8];
uint8_t RxData[8];
uint32_t TxMailbox;
volatile uint32_t echo_count = 0;
volatile uint16_t rpm=0, speed=0;
uint16_t arr_rpm=0;
uint16_t ccr_speed=0;
uint16_t arr_speed=0;
uint16_t ccr_rpm=0;

//====================CAN===========

//Thermals==============================
uint16_t adc_buff[ADC_BUFF_SIZE];
volatile uint16_t adc_val=0;
float voltage=0.0f;
float temp[4];


const float temp_breakpoints[Table_Size] = {0, 50, 55, 60, 65, 70, 75, 80, 85, 90};
const float mdot_data[Table_Size] = {0, 0.005, 0.01, 0.03, 0.06, 0.08, 0.1, 0.1, 0.15, 0.15};

// --- GLOBAL VARIABLES ---
volatile uint32_t adc_raw_value = 0;
float stator_temp = 0.0f;
float target_mdot = 0.0f;
uint32_t pwm_duty_cycle = 0;
//Thermals==========================

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_CAN_Init(void);
static void MX_I2C1_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

//===============================I2C=================
void MPU6050_ReadAll(I2C_HandleTypeDef *hi2c, int16_t *ax, int16_t *ay, int16_t *az,
                      int16_t *gx, int16_t *gy, int16_t *gz) {
    uint8_t buf[14];
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, ACCEL_XOUT_H, 1, buf, 14, 100);

    *ax = (int16_t)(buf[0]  << 8 | buf[1]);
    *ay = (int16_t)(buf[2]  << 8 | buf[3]);
    *az = (int16_t)(buf[4]  << 8 | buf[5]);
    // buf[6],buf[7] = temperature, skip or parse separately
    *gx = (int16_t)(buf[8]  << 8 | buf[9]);
    *gy = (int16_t)(buf[10] << 8 | buf[11]);
    *gz = (int16_t)(buf[12] << 8 | buf[13]);
}

void MPU6050_Init(I2C_HandleTypeDef *hi2c) {
    uint8_t check, data;

    // Verify device is alive — should read 0x68
    HAL_I2C_Mem_Read(hi2c, MPU6050_ADDR, WHO_AM_I, 1, &check, 1, 100);

    if (check == 0x68) {
        // Wake up: clear sleep bit, select internal 8MHz clock
        data = 0x00;
        HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, PWR_MGMT_1, 1, &data, 1, 100);

        // Sample rate divider: 1kHz / (1 + 7) = 125Hz
        data = 0x07;
        HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, SMPLRT_DIV, 1, &data, 1, 100);

        // Accel config: ±2g
        data = 0x00;
        HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, ACCEL_CONFIG, 1, &data, 1, 100);

        // Gyro config: ±250°/s
        data = 0x00;
        HAL_I2C_Mem_Write(hi2c, MPU6050_ADDR, GYRO_CONFIG, 1, &data, 1, 100);
    }
}

/* USER CODE BEGIN 0 */

// returns 1 on success, 0 on I2C error
uint8_t MLX90614_ReadTemp(I2C_HandleTypeDef *hi2c, uint8_t addr, uint8_t reg, float *temp_c) {
    uint8_t buf[3];  // data_low, data_high, PEC

    if (HAL_I2C_Mem_Read(hi2c, addr, reg, 1, buf, 3, 100) != HAL_OK) {
        return 0;
    }

    uint16_t raw = (uint16_t)(buf[1] << 8 | buf[0]);

    // MSB of high byte is an error flag on some regs — mask it off
    raw &= 0x7FFF;

    *temp_c = (raw * 0.02f) - 273.15f;
    return 1;
}


//===============================I2C=================


//===============CAN======================
static void CAN_Filter_Config(void)
{
CAN_FilterTypeDef sFilterConfig;
sFilterConfig.FilterBank = 0;
sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
sFilterConfig.FilterIdHigh = 0x0000;
sFilterConfig.FilterIdLow = 0x0000;
sFilterConfig.FilterMaskIdHigh = 0x0000;
sFilterConfig.FilterMaskIdLow = 0x0000;
sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
sFilterConfig.FilterActivation = ENABLE;
if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK) Error_Handler();
}
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *canHandle) {
    if (HAL_CAN_GetRxMessage(canHandle, CAN_RX_FIFO0, &RxHeader, RxData) == HAL_OK) {

        // Ensure it's an Extended ID
        if (RxHeader.IDE == CAN_ID_EXT) {

            // Sniff Motor Controller Data
            if (RxHeader.ExtId == 0x18FF2803) {
                // Parse AC Current / Voltage
            }
            else if (RxHeader.ExtId == 0x18FF2D03) {
                            // Parse incoming DC-Link Voltage
                            uint16_t dc_link_volts = (RxData[0] << 8) | RxData[1];

                            // Evaluate electrical power balance to set limits
                            // Example: Max limit is current DC-Link + 20V, Min is DC-Link - 20V
                            uint16_t max_batt_volts = dc_link_volts + 20;
                            uint16_t min_batt_volts = dc_link_volts - 20;

                            CAN_TxHeaderTypeDef RespHeader;
                            RespHeader.ExtId = 0x0CFF3A27; // Max/Min Battery Voltage ID
                            RespHeader.IDE = CAN_ID_EXT;
                            RespHeader.RTR = CAN_RTR_DATA;
                            RespHeader.DLC = 8;

                            uint8_t RespData[8] = {0};

                            // Pack Byte 1 & 2: Max Battery Voltage
                            RespData[0] = (max_batt_volts >> 8) & 0xFF;
                            RespData[1] = max_batt_volts & 0xFF;

                            // Pack Byte 3 & 4: Min Battery Voltage
                            RespData[2] = (min_batt_volts >> 8) & 0xFF;
                            RespData[3] = min_batt_volts & 0xFF;

                            // Remaining bytes stay 0x00 as per matrix

                            HAL_CAN_AddTxMessage(&hcan, &RespHeader, RespData, &TxMailbox);
                        }
        }
    }
}
//===============CAN========================

//---------------thermal------------
//Lookup Table Interpolation
float mdot_from_temp(float input_temp) {
	if (input_temp <= temp_breakpoints[0]) {
		return mdot_data[0];
	}
	if (input_temp >= temp_breakpoints[Table_Size - 1]) {
		return mdot_data[Table_Size - 1];
	}
	int i = 0;
	while (temp_breakpoints[i + 1] < input_temp) {
		i++;
	}

	float x0 = temp_breakpoints[i];
	float x1 = temp_breakpoints[i + 1];
	float y0 = mdot_data[i];
	float y1 = mdot_data[i + 1];

	return y0 + ((input_temp - x0) * ((y1 - y0) / (x1 - x0)));
}

// Map Flow Rate to PWM
uint32_t calculate_pwm_duty(float flow_rate) {
	if (flow_rate > MDOT_MAX_VAL) {
		flow_rate = MDOT_MAX_VAL;
	}
	if (flow_rate < 0.0f) {
		flow_rate = 0.0f;
	}
	return (uint32_t) ((flow_rate / MDOT_MAX_VAL) * PWM_PERIOD);
}

 float therm_to_temp(uint16_t adc_value){

	 float voltage = ((float) adc_value * 3.3) / 4095.0f;
	 if (voltage >= 3.29f)
	 {

	 voltage = 3.29f;
	 }
	 float resistance = (voltage * 10000.0f) / (3.3f - voltage) ;
	 float temperature = (-31.19 * logf (resistance)) + 313.73f;
	 return temperature;
 }
//------------thermal--------------------
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
  MX_CAN_Init();
  MX_I2C1_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  MPU6050_Init(&hi2c1);
  /* USER CODE END 2 */
  //=====================CAN==========================
   MX_TIM1_Init();
   TIM1->CCR1 = 0;
   HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_1);

   MX_TIM2_Init();
   TIM2->CCR1 = 0;
   HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
   CAN_Filter_Config();
     if (HAL_CAN_Start(&hcan) != HAL_OK) Error_Handler();
       if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) Error_Handler();

       TxHeader.DLC=8;
         TxHeader.IDE=CAN_ID_EXT;
         TxHeader.RTR=CAN_RTR_DATA;
     		        TxHeader.TransmitGlobalTime = DISABLE;

 //======================CAN============================

//Thermals=====================================
       HAL_ADCEx_Calibration_Start(&hadc1);
       __HAL_RCC_AFIO_CLK_ENABLE();
       //	__HAL_AFIO_REMAP_TIM1_PARTIAL();
       HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buff, ADC_BUFF_SIZE);
       HAL_TIM_PWM_Start(&htim1,TIM_CHANNEL_2);
       //HAL_TIMEx_PWMN_Start(&htim1, TIM_CHANNEL_2);
       __HAL_TIM_MOE_ENABLE(&htim1);
//Thermals=====================================
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		 //------------ Brake light -----------------


		 	  				adc_val = adc_buff[ADC_BRAKE_INDEX];
		 	  				voltage = ((float) adc_val * 3.3) / 4095.0f;
		 	  				if (adc_val > Brake_pedal_threshold) {
		 	  					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
		 	  				} else {
		 	  					HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
		 	  				}
		 	  				HAL_Delay(5);
		 //---------------brake light --------------------------


	  //===============================I2C=================
	  MPU6050_ReadAll(&hi2c1,&mpu_buf[0],&mpu_buf[1],&mpu_buf[2],&mpu_buf[3],&mpu_buf[4],&mpu_buf[5]);

	 	  MLX90614_ReadTemp(&hi2c1, MLX90614_ADDR1, MLX90614_TA,    &temp_ambient1);
	 	  	  MLX90614_ReadTemp(&hi2c1, MLX90614_ADDR1, MLX90614_TOBJ1, &temp_obj1);
	 	  	  MLX90614_ReadTemp(&hi2c1, MLX90614_ADDR2, MLX90614_TA,    &temp_ambient2);
	 	  	  MLX90614_ReadTemp(&hi2c1, MLX90614_ADDR2, MLX90614_TOBJ1, &temp_obj2);
	 	  	  HAL_Delay(1000);
	 		  //===============================I2C=================

	 //=========================CAN=================================
	 	  	TxHeader.ExtId = 0x18FA2803;
	 	  		      uint8_t Data_Thermal[8] = {0x11, 0x11, 0x22, 0x22, 0x00, 0x00, 0x00, 0x00};
	 	  		      HAL_CAN_AddTxMessage(&hcan, &TxHeader, Data_Thermal, &TxMailbox);
	 	  		      HAL_Delay(1);

	 	  		      // Send Brake Pressure
	 	  		      TxHeader.ExtId = 0x18FA2903;
	 	  		      uint8_t Data_Brake[8] = {0xAA, 0xBB, 0x00, 0x00, 0xCC, 0xDD, 0x00, 0x00};
	 	  		      HAL_CAN_AddTxMessage(&hcan, &TxHeader, Data_Brake, &TxMailbox);
	 	  		      HAL_Delay(1);
	 //=======================CAN======================================



	 //---------------thermal---------------------

	 	  				// READ SENSOR
	 	  				float temp_1 = therm_to_temp(adc_buff[ADC_THERM1_INDEX]);
	 	  				float temp_2 = therm_to_temp(adc_buff[ADC_THERM2_INDEX]);
	 	  				float temp_3 = therm_to_temp(adc_buff[ADC_THERM3_INDEX]);
	 	  				float temp_4 = therm_to_temp(adc_buff[ADC_THERM4_INDEX]);
	 	  				temp[0]=temp_1;
	 	  				temp[1]=temp_2;
	 	  				temp[2]=temp_3;
	 	  				temp[3]=temp_4;
	 	  				stator_temp = temp_1;
	 	  				if (temp_2 > stator_temp) {
	 	  					stator_temp = temp_2;
	 	  				}
	 	  				if (temp_3 > stator_temp) {
	 	  					stator_temp = temp_3;
	 	  				}
	 	  				if (temp_4 > stator_temp) {
	 	  					stator_temp = temp_4;
	 	  				}

	 	  				// LOGIC (Lookup Table)
	 	  				target_mdot = mdot_from_temp(stator_temp);

	 	  				// PWM generation
	 	  				pwm_duty_cycle = calculate_pwm_duty(target_mdot);
	 	  				__HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_2, pwm_duty_cycle);
	 	//------------------thermal------------
	 	  			}
    /* USER CODE BEGIN 3 */
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
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 5;
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

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_6;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Rank = ADC_REGULAR_RANK_7;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CAN Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN_Init(void)
{

  /* USER CODE BEGIN CAN_Init 0 */

  /* USER CODE END CAN_Init 0 */

  /* USER CODE BEGIN CAN_Init 1 */

  /* USER CODE END CAN_Init 1 */
  hcan.Instance = CAN1;
  hcan.Init.Prescaler = 4;
  hcan.Init.Mode = CAN_MODE_NORMAL;
  hcan.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan.Init.TimeSeg1 = CAN_BS1_15TQ;
  hcan.Init.TimeSeg2 = CAN_BS2_2TQ;
  hcan.Init.TimeTriggeredMode = DISABLE;
  hcan.Init.AutoBusOff = DISABLE;
  hcan.Init.AutoWakeUp = DISABLE;
  hcan.Init.AutoRetransmission = DISABLE;
  hcan.Init.ReceiveFifoLocked = DISABLE;
  hcan.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN_Init 2 */

  /* USER CODE END CAN_Init 2 */

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
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 65535;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
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
  if( HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
   {
     Error_Handler();
   }
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

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 0;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
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
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel6_IRQn);
  /* DMA1_Channel7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel7_IRQn);

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
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
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
