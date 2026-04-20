/**
 ******************************************************************************
 * @file    main.c
 * @brief   Full duplex — mic recording + audio playback
 *
 * RECORD:   MAX9814 -> PA0 (ADC) -> TIM2 16kHz -> PA9 UART TX -> RPi
 * PLAYBACK: RPi -> PA10 UART RX -> sigma-delta -> PA8 speaker
 * DEBUG:    PA2 USART2 TX -> Mac via ST-Link
 *
 * Wiring:
 *   MAX9814 OUT  -> PA0  (CN7  pin 28)
 *   MAX9814 VDD  -> 3.3V (CN6  pin 4)
 *   MAX9814 GND  -> GND  (CN6  pin 6)
 *   PA9  (CN5  pin 1)   -> RPi Pin 10 (RX)   mic audio out
 *   PA10 (CN10 pin 33)  -> RPi Pin 8  (TX)   playback audio in
 *   PA8  (CN10 pin 23)  -> Speaker +
 *   GND  (CN6  pin 6)   -> Speaker -
 ******************************************************************************
 */

#include "main.h"
#include <stdint.h>
#include <stdio.h>

/* ── Audio config ─────────────────────────────────────────────────────────── */
#define HALF_BUF_SAMPLES    256U
#define FULL_BUF_SAMPLES    (HALF_BUF_SAMPLES * 2)
#define UART_TX_HALF        (HALF_BUF_SAMPLES * 2)
#define DC_BIAS_COUNTS      1551U

/* ── Handles ──────────────────────────────────────────────────────────────── */
ADC_HandleTypeDef  hadc1;
DMA_HandleTypeDef  hdma_adc1;
UART_HandleTypeDef huart1;   // RPi — PA9 TX, PA10 RX
UART_HandleTypeDef huart2;   // Mac debug — PA2 TX
DMA_HandleTypeDef  hdma_usart1_tx;

/* ── Record buffers ───────────────────────────────────────────────────────── */
static uint16_t adc_buf[FULL_BUF_SAMPLES];
static uint8_t  uart_tx_a[UART_TX_HALF];
static uint8_t  uart_tx_b[UART_TX_HALF];

/* ── Flags ────────────────────────────────────────────────────────────────── */
static volatile uint8_t adc_half_rdy = 0;
static volatile uint8_t adc_full_rdy = 0;

/* ── Forward declarations ─────────────────────────────────────────────────── */
void SystemClock_Config(void);
void Error_Handler(void);
static void TIM2_Init_Bare(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_GPIO_Init(void);
static void ProcessAndSend(const uint16_t *raw, uint8_t *out);

/* ════════════════════════════════════════════════════════════════════════════
 * HAL_UART_MspInit
 ════════════════════════════════════════════════════════════════════════════ */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /* PA9=TX, PA10=RX */
        GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }

    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        /* PA2=TX to Mac */
        GPIO_InitStruct.Pin       = GPIO_PIN_2;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_NOPULL;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * HAL_ADC_MspInit
 ════════════════════════════════════════════════════════════════════════════ */
void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if (hadc->Instance == ADC1) {
        __HAL_RCC_ADC1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin  = GPIO_PIN_0;
        GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * MX_GPIO_Init
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* PA8 = speaker output — max speed for sigma-delta */
    GPIO_InitStruct.Pin   = GPIO_PIN_8;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA5 = onboard LED */
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
}

/* ════════════════════════════════════════════════════════════════════════════
 * TIM2_Init_Bare — 16kHz ADC trigger, no HAL TIM needed
 ════════════════════════════════════════════════════════════════════════════ */
static void TIM2_Init_Bare(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __DSB();
    TIM2->CR1  = 0;
    TIM2->PSC  = 0;
    TIM2->ARR  = 5249;           // 84MHz / 5250 = 16kHz
    TIM2->CNT  = 0;
    TIM2->CR2  = TIM_CR2_MMS_1; // TRGO = update event
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0;
    TIM2->CR1  = TIM_CR1_CEN;
}

/* ════════════════════════════════════════════════════════════════════════════
 * ProcessAndSend — ADC samples -> 2-byte frames -> RPi via UART TX DMA
 ════════════════════════════════════════════════════════════════════════════ */
static void ProcessAndSend(const uint16_t *raw, uint8_t *out) {
    for (uint32_t i = 0; i < HALF_BUF_SAMPLES; i++) {
        int32_t s = (int32_t)raw[i] - DC_BIAS_COUNTS;
        if      (s >  2047) s =  2047;
        else if (s < -2048) s = -2048;
        uint16_t v = (uint16_t)(s + 2048);
        out[i * 2 + 0] = 0x80 | (v >> 6);
        out[i * 2 + 1] =         v & 0x3F;
    }
    if (HAL_UART_GetState(&huart1) == HAL_UART_STATE_READY) {
        HAL_UART_Transmit_DMA(&huart1, out, UART_TX_HALF);
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * ADC DMA Callbacks
 ════════════════════════════════════════════════════════════════════════════ */
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) adc_half_rdy = 1;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
    if (hadc->Instance == ADC1) adc_full_rdy = 1;
}

/* ════════════════════════════════════════════════════════════════════════════
 * main
 ════════════════════════════════════════════════════════════════════════════ */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART1_UART_Init();
    MX_USART2_UART_Init();
    MX_ADC1_Init();
    TIM2_Init_Bare();

    /* Banner on Mac */
    uint8_t banner[] = "=== RECORD + PLAYBACK READY ===\r\n";
    HAL_UART_Transmit(&huart2, banner, sizeof(banner)-1, 1000);

    /* 3 blinks = ready */
    for (int i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(200);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    /* Start mic ADC DMA */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, FULL_BUF_SAMPLES);

    uint8_t  b0, b1;
    uint32_t sample_count = 0;
    int32_t  accumulator  = 0;

    while (1) {
        /* ── RECORD PATH — send mic data to RPi ── */
        if (adc_half_rdy) {
            adc_half_rdy = 0;
            ProcessAndSend(&adc_buf[0], uart_tx_a);
        }
        if (adc_full_rdy) {
            adc_full_rdy = 0;
            ProcessAndSend(&adc_buf[HALF_BUF_SAMPLES], uart_tx_b);
        }

        /* ── PLAYBACK PATH — receive from RPi, drive speaker ── */
        if (HAL_UART_Receive(&huart1, &b0, 1, 1) == HAL_OK) {
            if (!(b0 & 0x80)) goto next; // resync

            if (HAL_UART_Receive(&huart1, &b1, 1, 1) != HAL_OK) goto next;
            if (b1 & 0x80) goto next;

            /* Decode */
            uint16_t val12 = ((b0 & 0x3F) << 6) | (b1 & 0x3F);
            int32_t  pcm   = (int32_t)((val12 - 2048) << 4);

            /* Sigma-delta 8x oversampling */
            for (int os = 0; os < 8; os++) {
                accumulator += pcm;
                if (accumulator >= 0) {
                    GPIOA->BSRR = GPIO_PIN_8;
                    accumulator -= 32767;
                } else {
                    GPIOA->BSRR = GPIO_PIN_8 << 16;
                    accumulator += 32767;
                }
                for (volatile uint32_t d = 0; d < 164; d++);
            }

            sample_count++;
            if (sample_count % 16000 == 0) {
                uint8_t msg[40];
                int len = snprintf((char*)msg, sizeof(msg),
                                   "[%lu samples]\r\n", sample_count);
                HAL_UART_Transmit(&huart2, msg, len, 100);
            }
        }
        next:;
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * SystemClock_Config — 84MHz
 ════════════════════════════════════════════════════════════════════════════ */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 16;
    RCC_OscInitStruct.PLL.PLLN            = 336;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ            = 4;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                       RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
}

/* ════════════════════════════════════════════════════════════════════════════
 * MX_ADC1_Init
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_ADC1_Init(void) {
    ADC_ChannelConfTypeDef sConfig = {0};

    hadc1.Instance                   = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode          = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_EXTERNALTRIGCONV_T2_TRGO;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DMAContinuousRequests = ENABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) Error_Handler();

    sConfig.Channel      = ADC_CHANNEL_0;
    sConfig.Rank         = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_84CYCLES;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK) Error_Handler();
}

/* ════════════════════════════════════════════════════════════════════════════
 * MX_DMA_Init
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* ADC1 -> DMA2 Stream0 Channel0 */
    hdma_adc1.Instance                 = DMA2_Stream0;
    hdma_adc1.Init.Channel             = DMA_CHANNEL_0;
    hdma_adc1.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode                = DMA_CIRCULAR;
    hdma_adc1.Init.Priority            = DMA_PRIORITY_HIGH;
    hdma_adc1.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    /* USART1 TX -> DMA2 Stream7 Channel4 */
    hdma_usart1_tx.Instance                 = DMA2_Stream7;
    hdma_usart1_tx.Init.Channel             = DMA_CHANNEL_4;
    hdma_usart1_tx.Init.Direction           = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment    = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode                = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority            = DMA_PRIORITY_MEDIUM;
    hdma_usart1_tx.Init.FIFOMode            = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK) Error_Handler();
    __HAL_LINKDMA(&huart1, hdmatx, hdma_usart1_tx);
    HAL_NVIC_SetPriority(DMA2_Stream7_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(DMA2_Stream7_IRQn);
}

/* ════════════════════════════════════════════════════════════════════════════
 * MX_USART1_UART_Init — 921600 baud TX+RX to RPi
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_USART1_UART_Init(void) {
    huart1.Instance          = USART1;
    huart1.Init.BaudRate     = 921600;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_8;
    if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

/* ════════════════════════════════════════════════════════════════════════════
 * MX_USART2_UART_Init — 115200 TX to Mac via ST-Link
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_USART2_UART_Init(void) {
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

/* ════════════════════════════════════════════════════════════════════════════
 * IRQ Handlers
 ════════════════════════════════════════════════════════════════════════════ */
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_adc1);      }
void DMA2_Stream7_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart1_tx); }
void USART1_IRQHandler(void)       { HAL_UART_IRQHandler(&huart1);        }

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
