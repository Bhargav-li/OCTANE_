/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    main.c
  * @brief   CAN TRANSMITTER — STM32F103C8T6
  *          500 kbps, PB8=CAN_RX, PB9=CAN_TX (AFIO remap)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan;

CAN_TxHeaderTypeDef TxHeader;
uint8_t  TxData[8];
uint32_t TxMailbox;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CAN_Init(void);

/* ── Main ────────────────────────────────────────────────────────────────── */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_CAN_Init();

    /* ── CAN Filter: pass all frames into FIFO0 ── */
    CAN_FilterTypeDef f = {0};
    f.FilterActivation     = CAN_FILTER_ENABLE;
    f.FilterBank           = 0;
    f.FilterFIFOAssignment = CAN_RX_FIFO0;
    f.FilterIdHigh         = 0x0000;
    f.FilterIdLow          = 0x0000;
    f.FilterMaskIdHigh     = 0x0000;
    f.FilterMaskIdLow      = 0x0000;
    f.FilterMode           = CAN_FILTERMODE_IDMASK;
    f.FilterScale          = CAN_FILTERSCALE_32BIT;
    f.SlaveStartFilterBank = 14;
    if (HAL_CAN_ConfigFilter(&hcan, &f) != HAL_OK) Error_Handler();

    /* ── Start CAN ── */
    if (HAL_CAN_Start(&hcan) != HAL_OK) Error_Handler();

    /* ── TX Header (fixed fields) ── */
    TxHeader.StdId = 0x201;          /* Status frame ID — RPi expects this */
    TxHeader.IDE   = CAN_ID_STD;
    TxHeader.RTR   = CAN_RTR_DATA;
    TxHeader.DLC   = 3;
    TxHeader.TransmitGlobalTime = DISABLE;

    /* ── Infinite loop ── */
    while (1)
    {
        uint32_t uptime_s = HAL_GetTick() / 1000;

        TxData[0] = (uint8_t)(uptime_s);        /* uptime low byte  */
        TxData[1] = (uint8_t)(uptime_s >> 8);   /* uptime high byte */
        TxData[2] = 0x00;                        /* error code       */

        /* Wait for a free TX mailbox (max 10 ms) */
        uint32_t t = HAL_GetTick();
        while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0)
        {
            if (HAL_GetTick() - t > 10) break;
        }

        HAL_StatusTypeDef ret =
            HAL_CAN_AddTxMessage(&hcan, &TxHeader, TxData, &TxMailbox);

        /* PC13 LED: blink = TX ok, solid on = TX fail */
        if (ret == HAL_OK)
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        else
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

        HAL_Delay(1000);
    }
}

/* ── System Clock: 72 MHz via HSE 8 MHz + PLL x9 ─────────────────────────── */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState            = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue      = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL          = RCC_PLL_MUL9;   /* 8 x 9 = 72 MHz */
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1 = 36 MHz */
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
        Error_Handler();
}

/* ── CAN Init: 500 kbps @ APB1=36 MHz ────────────────────────────────────────
 *
 *  Baud = APB1 / (Prescaler × (1 + BS1 + BS2))
 *       = 36 000 000 / (9 × (1 + 3 + 4))  — wait, let's be precise:
 *
 *  Target: 500 kbps, sample point ~75%
 *  Using:  Prescaler=9, BS1=CAN_BS1_4TQ, BS2=CAN_BS2_3TQ
 *  Total TQ = 1 + 4 + 3 = 8
 *  Baud = 36 MHz / (9 × 8) = 500 000 bps ✓
 *  Sample point = (1+4)/8 = 62.5%
 * ──────────────────────────────────────────────────────────────────────────── */
static void MX_CAN_Init(void)
{
    hcan.Instance                  = CAN1;
    hcan.Init.Prescaler            = 9;
    hcan.Init.Mode                 = CAN_MODE_NORMAL;   /* Normal bus mode */
    hcan.Init.SyncJumpWidth        = CAN_SJW_1TQ;
    hcan.Init.TimeSeg1             = CAN_BS1_4TQ;
    hcan.Init.TimeSeg2             = CAN_BS2_3TQ;
    hcan.Init.TimeTriggeredMode    = DISABLE;
    hcan.Init.AutoBusOff           = ENABLE;            /* Auto recover from BUS-OFF */
    hcan.Init.AutoWakeUp           = DISABLE;
    hcan.Init.AutoRetransmission   = ENABLE;
    hcan.Init.ReceiveFifoLocked    = DISABLE;
    hcan.Init.TransmitFifoPriority = DISABLE;
    if (HAL_CAN_Init(&hcan) != HAL_OK) Error_Handler();
}

/* ── GPIO Init ────────────────────────────────────────────────────────────────
 *  PC13 — onboard LED (active LOW)
 *  PB8  — CAN_RX  (AF remapped)
 *  PB9  — CAN_TX  (AF remapped, push-pull)
 * ──────────────────────────────────────────────────────────────────────────── */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_AFIO_CLK_ENABLE();    /* Must enable AFIO for remap */

    /* ── CRITICAL: Remap CAN1 from PA11/PA12 → PB8/PB9 ── */
    __HAL_AFIO_REMAP_CAN1_2();

    /* PC13 — LED output */
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); /* LED off initially */
    GPIO_InitStruct.Pin   = GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    /* PB8 — CAN_RX (input floating) */
    GPIO_InitStruct.Pin  = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* PB9 — CAN_TX (alternate function push-pull) */
    GPIO_InitStruct.Pin   = GPIO_PIN_9;
    GPIO_InitStruct.Mode  = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* ── Error Handler ───────────────────────────────────────────────────────── */
void Error_Handler(void)
{
    __disable_irq();
    /* Rapid blink to signal error */
    while (1)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        for (volatile uint32_t i = 0; i < 200000; i++);
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) { }
#endif
