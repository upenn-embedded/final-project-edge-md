
/**
 ******************************************************************************
 * @file    main.c
 * @brief   MAX98357A I²S — narrowing down where init fails
 *
 * LED pattern at boot:
 *   1 blink  = clocks OK
 *   2 blinks = PLLI2S ENABLE call returned
 *   3 blinks = PLLI2S locked
 *   4 blinks = SPI2 clock enabled
 *   5 blinks = MX_I2S2_Init ENTERED
 *   6 blinks = HAL_I2S_Init returned OK  (← this is where we've been failing)
 *   7 blinks = First transmit OK
 *   Solid ON = failure
 ******************************************************************************
 */

#include "main.h"
#include <math.h>
#include <stdint.h>

#define SAMPLE_RATE  16000U
#define PI           3.14159265f
#define BUF_SAMPLES  1024U

I2S_HandleTypeDef hi2s2;
static int16_t audio_buf[BUF_SAMPLES];

void SystemClock_Config(void);
void Error_Handler(void);
static void BlinkN(int n, uint32_t ms);

/* ── I²S MspInit ──────────────────────────────────────────────────────────── */
void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s) {
    GPIO_InitTypeDef g = {0};
    if (hi2s->Instance == SPI2) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
        g.Pin       = GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_15;
        g.Mode      = GPIO_MODE_AF_PP;
        g.Pull      = GPIO_NOPULL;
        g.Speed     = GPIO_SPEED_FREQ_HIGH;
        g.Alternate = GPIO_AF5_SPI2;
        HAL_GPIO_Init(GPIOB, &g);
    }
}

void SystemClock_Config(void) {
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = 16; osc.PLL.PLLN = 336; osc.PLL.PLLP = RCC_PLLP_DIV4; osc.PLL.PLLQ = 4;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) Error_Handler();
}

void Error_Handler(void) {
    __disable_irq();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin = GPIO_PIN_5;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &g);
    GPIOA->BSRR = GPIO_PIN_5;
    while (1) {}
}

static void BlinkN(int n, uint32_t ms) {
    for (int i = 0; i < n; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
        HAL_Delay(ms);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
        HAL_Delay(ms);
    }
    HAL_Delay(700);  /* Clear pause between checkpoint groups */
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* Setup LED */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin = GPIO_PIN_5;
    led.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &led);

    BlinkN(1, 200);   /* Checkpoint 1 */

    /* Disable PLLI2S first */
    __HAL_RCC_PLLI2S_DISABLE();
    for (volatile int i = 0; i < 1000; i++);

    /* Set PLLI2S config directly */
    RCC->PLLI2SCFGR = (192U << RCC_PLLI2SCFGR_PLLI2SN_Pos) |
                      (2U   << RCC_PLLI2SCFGR_PLLI2SR_Pos);

    /* Enable PLLI2S */
    __HAL_RCC_PLLI2S_ENABLE();

    BlinkN(2, 200);   /* Checkpoint 2: PLLI2S enabled */

    /* Wait for PLLI2S lock */
    uint32_t timeout = HAL_GetTick() + 500;
    while ((RCC->CR & RCC_CR_PLLI2SRDY) == 0) {
        if (HAL_GetTick() > timeout) Error_Handler();
    }

    BlinkN(3, 200);   /* Checkpoint 3: PLLI2S locked */

    /* Enable SPI2 clock */
    __HAL_RCC_SPI2_CLK_ENABLE();

    BlinkN(4, 200);   /* Checkpoint 4: SPI2 clock on */

    /* Configure I²S handle and call init */
    hi2s2.Instance            = SPI2;
    hi2s2.Init.Mode           = I2S_MODE_MASTER_TX;
    hi2s2.Init.Standard       = I2S_STANDARD_PHILIPS;
    hi2s2.Init.DataFormat     = I2S_DATAFORMAT_16B;
    hi2s2.Init.MCLKOutput     = I2S_MCLKOUTPUT_DISABLE;
    hi2s2.Init.AudioFreq      = I2S_AUDIOFREQ_16K;
    hi2s2.Init.CPOL           = I2S_CPOL_LOW;
    hi2s2.Init.ClockSource    = I2S_CLOCK_PLL;
    hi2s2.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;

    BlinkN(5, 200);   /* Checkpoint 5: about to call HAL_I2S_Init */

    HAL_StatusTypeDef init_result = HAL_I2S_Init(&hi2s2);
    if (init_result != HAL_OK) Error_Handler();

    BlinkN(6, 200);   /* Checkpoint 6: HAL_I2S_Init returned OK */

    /* Fill buffer with simple pattern */
    for (uint32_t i = 0; i < BUF_SAMPLES; i++) {
        audio_buf[i] = (i & 1) ? 10000 : -10000;
    }

    HAL_StatusTypeDef tx_result = HAL_I2S_Transmit(&hi2s2, (uint16_t*)audio_buf, BUF_SAMPLES, HAL_MAX_DELAY);
    if (tx_result != HAL_OK) Error_Handler();

    BlinkN(7, 200);   /* Checkpoint 7: first transmit done */

    /* Hammer forever */
    while (1) {
        HAL_I2S_Transmit(&hi2s2, (uint16_t*)audio_buf, BUF_SAMPLES, HAL_MAX_DELAY);
    }
}
