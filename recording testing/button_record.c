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
 *
 * Hardware connections (Nucleo-F411RE):
 *   PA2  — USART2 TX  →  RPi RX  (/dev/ttyAMA0)
 *   PA3  — USART2 RX  ←  RPi TX  (unused by this firmware)
 *   PC13 — USER button (active-low, internal pull-up enabled)
 *   PA5  — LD2 LED (blinks on each control byte sent)
 ******************************************************************************
 */

#include "main.h"

/* ── Peripherals ──────────────────────────────────────────────────────────── */
UART_HandleTypeDef huart2;

/* ── Prototypes ───────────────────────────────────────────────────────────── */
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void SendControlSeq(uint8_t cmd);

/* ── Control byte escape sequence ────────────────────────────────────────── */
/*
 * 0xFE 0xFE <cmd>  is unambiguous in the audio frame protocol:
 *   audio byte-0 has bit-7 SET  (0x80-0xFF)
 *   audio byte-1 has bit-7 CLEAR (0x00-0x7F)
 * Two consecutive 0xFE bytes cannot appear in a valid audio frame pair,
 * so the RPi decoder can safely distinguish control packets from PCM data.
 */
#define CTRL_PREFIX_A  0xFE
#define CTRL_PREFIX_B  0xFE
#define CMD_START      0x53   /* 'S' */
#define CMD_STOP       0x45   /* 'E' */

/* ── LED blink helper ─────────────────────────────────────────────────────── */
static inline void BlinkLED(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    HAL_Delay(30);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

/* ── Main ─────────────────────────────────────────────────────────────────── */
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();

    /* Initial LED off */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    GPIO_PinState prev = GPIO_PIN_SET;  /* released = high (active-low button) */

    while (1) {
        GPIO_PinState cur = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

        if (cur != prev) {
            HAL_Delay(20);  /* debounce window */
            GPIO_PinState confirmed = HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13);

            if (confirmed == cur) {
                if (cur == GPIO_PIN_RESET) {
                    /* Falling edge: button pressed → START recording */
                    SendControlSeq(CMD_START);
                    BlinkLED();
                } else {
                    /* Rising edge: button released → STOP recording */
                    SendControlSeq(CMD_STOP);
                    BlinkLED();
                }
                prev = cur;
            }
        }

        HAL_Delay(5);  /* polling interval */
    }
}

/* ── Send 3-byte control sequence ────────────────────────────────────────── */
static void SendControlSeq(uint8_t cmd) {
    uint8_t buf[3] = { CTRL_PREFIX_A, CTRL_PREFIX_B, cmd };
    HAL_UART_Transmit(&huart2, buf, 3, 100);
}

/* ── GPIO Init ───────────────────────────────────────────────────────────── */
static void MX_GPIO_Init(void) {
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef g = {0};

    /* PA5 — LED output */
    g.Pin   = GPIO_PIN_5;
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &g);

    /* PC13 — USER button, input with pull-up (active-low on Nucleo) */
    g.Pin  = GPIO_PIN_13;
    g.Mode = GPIO_MODE_INPUT;
    g.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOC, &g);
}

/* ── USART2 Init (921600 8N1, PA2 TX / PA3 RX) ───────────────────────────── */
static void MX_USART2_UART_Init(void) {
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure PA2 (TX) and PA3 (RX) as AF7 (USART2) */
    GPIO_InitTypeDef g = {0};
    g.Pin       = GPIO_PIN_2 | GPIO_PIN_3;
    g.Mode      = GPIO_MODE_AF_PP;
    g.Pull      = GPIO_NOPULL;
    g.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    g.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &g);

    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 921600;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart2) != HAL_OK) Error_Handler();
}

/* ── Clock Config (same HSI/PLL as EdgMD main.c → 84 MHz SYSCLK) ─────────── */
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
    osc.PLL.PLLM            = 16;
    osc.PLL.PLLN            = 336;
    osc.PLL.PLLP            = RCC_PLLP_DIV4;
    osc.PLL.PLLQ            = 4;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

    clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                       | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
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
