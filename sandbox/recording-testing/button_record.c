/**
 ******************************************************************************
 * @file    button_record.c
 * @brief   Hold USER button (PC13) to record; release to stop.
 *
 * Drop this file into the EdgMD project as a replacement for main.c.
 *
 * What this firmware does:
 *   - Monitors the onboard USER button (PC13, active-low)
 *   - On press  (falling edge): sends 0xFE 0xFE 0x53 ('S') via USART2
 *   - On release (rising edge): sends 0xFE 0xFE 0x45 ('E') via USART2
 *   - 20 ms software debounce on every edge
 *   - Sends 0xAA at boot so you can verify TX is working before any button press
 *
 * Hardware connections (Nucleo-F411RE):
 *   PA2  — USART2 TX  →  RPi RX  (/dev/ttyAMA0)
 *   PA3  — USART2 RX  ←  RPi TX  (unused by this firmware)
 *   PC13 — USER button (active-low, internal pull-up enabled)
 *   PA5  — LD2 LED (blinks on each control byte sent)
 ******************************************************************************
 */

#include "main.h"

UART_HandleTypeDef huart2;

void SystemClock_Config(void);
void Error_Handler(void);
static void SendControlSeq(uint8_t cmd);

/* ── Main ─────────────────────────────────────────────────────────────────── */
int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* LED */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef led = {0};
    led.Pin   = GPIO_PIN_5;
    led.Mode  = GPIO_MODE_OUTPUT_PP;
    led.Pull  = GPIO_NOPULL;
    led.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &led);

    /* Button — PC13, input with pull-up (active-low on Nucleo) */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef btn = {0};
    btn.Pin  = GPIO_PIN_13;
    btn.Mode = GPIO_MODE_INPUT;
    btn.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &btn);

    /* USART2 — 921600 8N1 */
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 921600;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();

    /* Boot marker — lets you verify TX works before touching the button.
       Run:  cat /dev/ttyAMA0 | xxd   and look for "aa" on startup. */
    uint8_t boot_marker = 0xAA;
    HAL_UART_Transmit(&huart2, &boot_marker, 1, 100);

    GPIO_PinState prev = GPIO_PIN_SET;  /* released = high (active-low) */

    while (1) {
        GPIO_PinState cur = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

        if (cur != prev) {
            HAL_Delay(20);  /* debounce */
            GPIO_PinState confirmed = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

            if (confirmed == cur) {
                if (cur == GPIO_PIN_RESET) {
                    /* Falling edge: button pressed → START */
                    SendControlSeq('S');
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                    HAL_Delay(50);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
                } else {
                    /* Rising edge: button released → STOP */
                    SendControlSeq('E');
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
                    HAL_Delay(50);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
                }
                prev = cur;
            }
        }

        HAL_Delay(5);
    }
}

/* ── Send 3-byte control escape sequence ─────────────────────────────────── */
static void SendControlSeq(uint8_t cmd) {
    uint8_t buf[3] = { 0xFE, 0xFE, cmd };
    HAL_UART_Transmit(&huart2, buf, 3, 100);
}

/* ── Clock Config (84 MHz SYSCLK from HSI, APB1 = 42 MHz for USART2) ─────── */
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

/* ── Error Handler ───────────────────────────────────────────────────────── */
void Error_Handler(void) {
    __disable_irq();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    GPIO_InitTypeDef g = {0};
    g.Pin  = GPIO_PIN_5;
    g.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(GPIOA, &g);
    GPIOA->BSRR = GPIO_PIN_5;  /* LED solid on = fault */
    while (1) {}
}
