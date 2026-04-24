/**
 ******************************************************************************
 * @file    main.c
 * @brief   Full duplex — mic recording (HAL) + I2S speaker playback (bare metal)
 *
 * RECORD:   MAX9814 -> PA0 (ADC) -> TIM2 16kHz -> PA9 UART TX -> RPi
 * PLAYBACK: RPi -> PA10 UART RX -> ring buf -> SPI3/I2S3 -> PA4/PC10/PC12
 * DEBUG:    PA2 USART2 TX -> Mac via ST-Link
 *
 * Wiring:
 *   MAX9814 OUT  -> PA0  (CN7 pin 28)
 *   MAX9814 VDD  -> 3.3V
 *   MAX9814 GND  -> GND
 *   PA9  (CN5 pin 1)   -> RPi Pin 10 (RX)
 *   PA10 (CN10 pin 33) -> RPi Pin 8  (TX)
 *   PA4  (WS)  -> I2S DAC LRCK
 *   PC10 (CK)  -> I2S DAC BCLK
 *   PC12 (SD)  -> I2S DAC DIN
 *
 * Pi-side (16 kHz mono 16-bit raw):
 *   ffmpeg -i input.wav -f s16le -ar 16000 -ac 1 -acodec pcm_s16le out.raw
 *   stty -F /dev/serial0 921600 raw -echo -echoe -echok -echoctl -echoke
 *   cat out.raw > /dev/serial0
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

/* ── Playback ring buffer (bytes) ─────────────────────────────────────────── */
/* Must be power of 2 for fast mask. 4096 = ~64 ms @ 16 kHz mono 16-bit */
#define RX_RING_SIZE        4096U
#define RX_RING_MASK        (RX_RING_SIZE - 1U)
static volatile uint8_t  rx_ring[RX_RING_SIZE];
static volatile uint32_t rx_head = 0;   /* written by ISR        */
static volatile uint32_t rx_tail = 0;   /* read by main loop     */

/* ── Playback gain (matches your example: *8) ─────────────────────────────── */
#define PLAYBACK_GAIN_SHIFT 3   /* <<3 = *8, i.e. +18 dB */

/* ── Handles (record path uses HAL) ───────────────────────────────────────── */
ADC_HandleTypeDef  hadc1;
DMA_HandleTypeDef  hdma_adc1;
UART_HandleTypeDef huart1;   /* RPi — PA9 TX, PA10 RX */
UART_HandleTypeDef huart2;   /* Mac debug — PA2 TX */
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
static void I2S3_Init_Bare(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_GPIO_Init(void);
static void ProcessAndSend(const uint16_t *raw, uint8_t *out);

/* ════════════════════════════════════════════════════════════════════════════
 * HAL_UART_MspInit — USART1 (PA9 TX, PA10 RX) + USART2 (PA2 TX debug)
 ════════════════════════════════════════════════════════════════════════════ */
void HAL_UART_MspInit(UART_HandleTypeDef *huart) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1) {
        __HAL_RCC_USART1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
        GPIO_InitStruct.Pin       = GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull      = GPIO_PULLUP;
        GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* Enable USART1 RX interrupt for ring buffer fill */
        HAL_NVIC_SetPriority(USART1_IRQn, 2, 0);  /* lower than ADC DMA */
        HAL_NVIC_EnableIRQ(USART1_IRQn);
    }

    if (huart->Instance == USART2) {
        __HAL_RCC_USART2_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();
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
 * MX_GPIO_Init — LED + I2S pins (PA4, PC10, PC12)
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_GPIO_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PA5 = onboard LED */
    GPIO_InitStruct.Pin   = GPIO_PIN_5;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PA4 = I2S3_WS  (AF6) */
    GPIO_InitStruct.Pin       = GPIO_PIN_4;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* PC10 = I2S3_CK, PC12 = I2S3_SD  (AF6) */
    GPIO_InitStruct.Pin       = GPIO_PIN_10 | GPIO_PIN_12;
    GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull      = GPIO_NOPULL;
    GPIO_InitStruct.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF6_SPI3;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

/* ════════════════════════════════════════════════════════════════════════════
 * TIM2_Init_Bare — 16 kHz ADC trigger
 ════════════════════════════════════════════════════════════════════════════ */
static void TIM2_Init_Bare(void) {
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    __DSB();
    TIM2->CR1  = 0;
    TIM2->PSC  = 0;
    TIM2->ARR  = 5249;                /* 84 MHz / 5250 = 16 kHz */
    TIM2->CNT  = 0;
    TIM2->CR2  = TIM_CR2_MMS_1;
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0;
    TIM2->CR1  = TIM_CR1_CEN;
}

/* ════════════════════════════════════════════════════════════════════════════
 * I2S3_Init_Bare — PLLI2S 96 MHz, 16-bit Philips, Master TX, ~16 kHz
 *
 * With PLLI2S = 96 MHz and I2SPR = 93 | ODD:
 *   Fs = 96e6 / (32 * (2*93 + 1) * 2) = ~8.04 kHz per channel? Let's verify:
 *   Actually for 16-bit data (no MCLK): Fs = PLLI2S_R / (32 * (2*I2SDIV + ODD))
 *   = 96e6 / (32 * 187) = ~16.04 kHz ✓ (matches your example's working config)
 *
 * Note: PLLI2S config happens in SystemClock_Config (separate from main PLL).
 ════════════════════════════════════════════════════════════════════════════ */
static void I2S3_Init_Bare(void) {
    /* Enable SPI3 clock */
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    __DSB();

    /* Disable I2S before configuring */
    SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;

    /* Prescaler: DIV=93, ODD=1 → effective divisor = 187 */
    SPI3->I2SPR = 93 | SPI_I2SPR_ODD;

    /* I2SMOD=1 (I2S mode), I2SCFG=10 (Master TX),
       I2SSTD=00 (Philips), DATLEN=00 (16-bit), CHLEN=0 (16-bit channel) */
    SPI3->I2SCFGR = SPI_I2SCFGR_I2SMOD
                  | (2U << SPI_I2SCFGR_I2SCFG_Pos);

    /* Enable I2S */
    SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;
}

/* ════════════════════════════════════════════════════════════════════════════
 * ProcessAndSend — ADC samples -> UART TX DMA (record path, unchanged)
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
 * USART1 IRQ — raw RXNE handler, fills ring buffer
 *
 * We bypass HAL's state machine here because HAL_UART_Receive_IT() reads a
 * fixed count then stops. We want continuous streaming.
 ════════════════════════════════════════════════════════════════════════════ */
void USART1_IRQHandler(void) {
    uint32_t sr = USART1->SR;

    if (sr & USART_SR_RXNE) {
        uint8_t byte = (uint8_t)(USART1->DR & 0xFF);
        uint32_t next = (rx_head + 1U) & RX_RING_MASK;
        if (next != rx_tail) {           /* drop byte if ring full */
            rx_ring[rx_head] = byte;
            rx_head = next;
        }
        /* else: overrun — keep ISR fast, don't block */
    }

    /* Clear error flags if set (ORE, FE, NE, PE) by reading SR then DR */
    if (sr & (USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE)) {
        (void)USART1->DR;
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * ring_pop_sample — try to pull one int16_t (2 bytes LE) from ring
 *  Returns 1 on success, 0 if not enough bytes available.
 ════════════════════════════════════════════════════════════════════════════ */
static inline int ring_pop_sample(int16_t *out) {
    /* Need at least 2 bytes */
    uint32_t avail = (rx_head - rx_tail) & RX_RING_MASK;
    if (avail < 2U) return 0;

    uint8_t lo = rx_ring[rx_tail];
    uint8_t hi = rx_ring[(rx_tail + 1U) & RX_RING_MASK];
    rx_tail = (rx_tail + 2U) & RX_RING_MASK;

    *out = (int16_t)((uint16_t)lo | ((uint16_t)hi << 8));
    return 1;
}

/* ════════════════════════════════════════════════════════════════════════════
 * i2s_write_stereo — push one mono sample to both L and R channels
 *
 * Matches your working example's frame layout: 4 half-word writes per stereo
 * frame (upper16, lower16, upper16, lower16). For 16-bit data, the "lower"
 * writes are zero padding.
 ════════════════════════════════════════════════════════════════════════════ */
static inline void i2s_write_stereo(int16_t sample) {
    /* Apply gain with saturation */
    int32_t amp = (int32_t)sample << PLAYBACK_GAIN_SHIFT;
    if      (amp >  32767) amp =  32767;
    else if (amp < -32768) amp = -32768;
    uint16_t s16 = (uint16_t)(int16_t)amp;

    /* LEFT channel: data, then zero padding */
    while (!(SPI3->SR & SPI_SR_TXE));
    SPI3->DR = s16;
    while (!(SPI3->SR & SPI_SR_TXE));
    SPI3->DR = 0x0000;

    /* RIGHT channel: same sample (mono → stereo duplicate) */
    while (!(SPI3->SR & SPI_SR_TXE));
    SPI3->DR = s16;
    while (!(SPI3->SR & SPI_SR_TXE));
    SPI3->DR = 0x0000;
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
    I2S3_Init_Bare();

    /* Enable USART1 RXNE interrupt manually (HAL didn't, since we don't use
       HAL_UART_Receive_IT) */
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

    /* Banner */
    uint8_t banner[] = "=== RECORD + I2S PLAYBACK READY ===\r\n";
    HAL_UART_Transmit(&huart2, banner, sizeof(banner) - 1, 1000);

    /* 3 blinks = ready */
    for (int i = 0; i < 3; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(200);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(200);
    }
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);

    /* Start mic ADC DMA (record path) */
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buf, FULL_BUF_SAMPLES);

    uint32_t sample_count   = 0;
    uint32_t underrun_count = 0;

    while (1) {
        /* ── RECORD PATH — mic -> Pi ── */
        if (adc_half_rdy) {
            adc_half_rdy = 0;
            ProcessAndSend(&adc_buf[0], uart_tx_a);
        }
        if (adc_full_rdy) {
            adc_full_rdy = 0;
            ProcessAndSend(&adc_buf[HALF_BUF_SAMPLES], uart_tx_b);
        }

        /* ── PLAYBACK PATH — Pi -> ring -> I2S ──
         *
         * The I2S TXE wait inside i2s_write_stereo paces us naturally to
         * 16 kHz. If the ring is empty, we emit silence (keeps DAC happy,
         * no audible pops from stale data).
         */
        int16_t sample;
        if (ring_pop_sample(&sample)) {
            i2s_write_stereo(sample);
        } else {
            i2s_write_stereo(0);
            underrun_count++;
        }

        sample_count++;

        /* Debug heartbeat: every ~1 sec @ 16 kHz */
        if (sample_count % 16000U == 0U) {
            uint32_t depth = (rx_head - rx_tail) & RX_RING_MASK;
            uint8_t msg[64];
            int len = snprintf((char *)msg, sizeof(msg),
                               "[s=%lu ring=%lu under=%lu]\r\n",
                               (unsigned long)sample_count,
                               (unsigned long)depth,
                               (unsigned long)underrun_count);
            HAL_UART_Transmit(&huart2, msg, len, 10);
        }
    }
}

/* ════════════════════════════════════════════════════════════════════════════
 * SystemClock_Config — 84 MHz SYSCLK + 96 MHz PLLI2S
 *
 * NOTE: HAL's RCC_OscInitStruct doesn't include PLLI2S, so we poke registers
 * directly after HAL_RCC_OscConfig. PLLI2SM uses the same M divider as the
 * main PLL on F4 (shared M), so PLLI2SM = 16 matches our PLLM = 16.
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

    /* PLLI2S: HSI (16 MHz) / M=16 * N=192 / R=2 = 96 MHz
     * Feeds SPI3/I2S3. MUST match your working example exactly. */
    RCC->CR &= ~RCC_CR_PLLI2SON;
    while (RCC->CR & RCC_CR_PLLI2SRDY);            /* wait for stop */
    RCC->PLLI2SCFGR = (16U  << 0)
                    | (192U << 6)
                    | (2U   << 28);
    RCC->CR |= RCC_CR_PLLI2SON;
    while (!(RCC->CR & RCC_CR_PLLI2SRDY));         /* wait for lock */
}

/* ════════════════════════════════════════════════════════════════════════════
 * MX_ADC1_Init — unchanged from your working version
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
 * MX_DMA_Init — unchanged (ADC + USART1 TX only, no RX DMA)
 ════════════════════════════════════════════════════════════════════════════ */
static void MX_DMA_Init(void) {
    __HAL_RCC_DMA2_CLK_ENABLE();

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
 * Note: USART1_IRQHandler is defined above (raw RXNE, bypasses HAL)
 ════════════════════════════════════════════════════════════════════════════ */
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_adc1);      }
void DMA2_Stream7_IRQHandler(void) { HAL_DMA_IRQHandler(&hdma_usart1_tx); }

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}
