/**
 ******************************************************************************
 * @file           : main_uart_rpi_rx.c
 * @brief          : Receive "hola como estas" from RPi5 over UART, mirror to Mac.
 *
 *   USART6 (RPi link): PA11 = TX  --> RPi5 RX (GPIO15 / header pin 10)
 *                      PA12 = RX  <-- RPi5 TX (GPIO14 / header pin 8)
 *                      GND  <->   GND
 *                      (USART6 uses AF8 on PA11/PA12, NOT AF7)
 *
 *   USART2 (Mac debug via ST-Link VCP):
 *                      PA2  = TX  --> Mac terminal
 *                      PA3  = RX
 *
 *   Both at 115200 8N1. STM32 runs on 16 MHz HSI after reset, so
 *   USARTDIV = 16e6 / (16 * 115200) ~= 8.68 -> BRR = 0x8B for both.
 *
 *   Match rule: every time a full line ('\n' terminated) arrives on USART6,
 *   we print `GOT FROM RPI: <line>` on USART2 and compare it against
 *   "hola como estas" -- on match, we print `MATCH!`.
 ******************************************************************************
 */

#include <stdint.h>

/* ---------- RCC ---------- */
#define RCC_AHB1ENR   (*(volatile uint32_t *)0x40023830)
#define RCC_APB1ENR   (*(volatile uint32_t *)0x40023840)
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40023844)

/* ---------- GPIOA ---------- */
#define GPIOA_MODER   (*(volatile uint32_t *)0x40020000)
#define GPIOA_AFRL    (*(volatile uint32_t *)0x40020020)  /* PA0..PA7  */
#define GPIOA_AFRH    (*(volatile uint32_t *)0x40020024)  /* PA8..PA15 */

/* ---------- USART2 (Mac debug) ---------- */
#define USART2_SR     (*(volatile uint32_t *)0x40004400)
#define USART2_DR     (*(volatile uint32_t *)0x40004404)
#define USART2_BRR    (*(volatile uint32_t *)0x40004408)
#define USART2_CR1    (*(volatile uint32_t *)0x4000440C)

/* ---------- USART6 (RPi link) ---------- */
#define USART6_SR     (*(volatile uint32_t *)0x40011400)
#define USART6_DR     (*(volatile uint32_t *)0x40011004)
#define USART6_BRR    (*(volatile uint32_t *)0x40011008)
#define USART6_CR1    (*(volatile uint32_t *)0x4001100C)

#define LINE_MAX 128
static char line_buf[LINE_MAX];
static uint32_t line_len = 0;

static const char expected[] = "hola como estas";

/* ---------- USART2 TX (to Mac) ---------- */
static void mac_putc(char c) {
    while (!(USART2_SR & (1 << 7))) { }   /* wait TXE */
    USART2_DR = (uint8_t)c;
}

static void mac_puts(const char *s) {
    while (*s) mac_putc(*s++);
}

/* ---------- USART6 RX (from RPi) ---------- */
static int rpi_has_byte(void) {
    return (USART6_SR & (1 << 5)) != 0;   /* RXNE */
}

static uint8_t rpi_read_byte(void) {
    return (uint8_t)USART6_DR;
}

/* ---------- Tiny string compare ---------- */
static int str_eq(const char *a, const char *b) {
    while (*a && *b) {
        if (*a != *b) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

int main(void) {
    /* Clocks: GPIOA, USART6 (APB2), USART2 (APB1) */
    RCC_AHB1ENR |= (1 << 0);          /* GPIOAEN  */
    RCC_APB2ENR |= (1 << 5);          /* USART6EN (APB2 bit 5) */
    RCC_APB1ENR |= (1 << 17);         /* USART2EN */

    /* PA2 = USART2 TX (AF7) */
    GPIOA_MODER &= ~(3 << (2 * 2));
    GPIOA_MODER |=  (2 << (2 * 2));   /* alt function */
    GPIOA_AFRL  &= ~(0xF << (2 * 4));
    GPIOA_AFRL  |=  (7   << (2 * 4));

    /* PA3 = USART2 RX (AF7) -- configured but unused */
    GPIOA_MODER &= ~(3 << (3 * 2));
    GPIOA_MODER |=  (2 << (3 * 2));
    GPIOA_AFRL  &= ~(0xF << (3 * 4));
    GPIOA_AFRL  |=  (7   << (3 * 4));

    /* PA11 = USART6 TX (AF8) -- configured, will be used for replies */
    GPIOA_MODER &= ~(3 << (11 * 2));
    GPIOA_MODER |=  (2 << (11 * 2));
    GPIOA_AFRH  &= ~(0xF << ((11 - 8) * 4));
    GPIOA_AFRH  |=  (8   << ((11 - 8) * 4));

    /* PA12 = USART6 RX (AF8) -- this is the one we care about */
    GPIOA_MODER &= ~(3 << (12 * 2));
    GPIOA_MODER |=  (2 << (12 * 2));
    GPIOA_AFRH  &= ~(0xF << ((12 - 8) * 4));
    GPIOA_AFRH  |=  (8   << ((12 - 8) * 4));

    /* 115200 baud @ 16 MHz HSI: BRR = 0x8B for both */
    USART2_BRR = 0x8B;
    USART2_CR1 = (1 << 13) | (1 << 3);               /* UE + TE        */

    USART6_BRR = 0x8B;
    USART6_CR1 = (1 << 13) | (1 << 2);               /* UE + RE        */

    mac_puts("\r\n--- RPi UART RX ready, expecting \"hola como estas\" ---\r\n");

    while (1) {
        if (!rpi_has_byte()) continue;

        uint8_t c = rpi_read_byte();

        /* Also mirror the raw byte to the Mac so you see it as it arrives. */
        mac_putc((char)c);

        if (c == '\r') continue;

        if (c == '\n' || line_len >= LINE_MAX - 1) {
            line_buf[line_len] = 0;

            mac_puts("\r\nGOT FROM RPI: ");
            mac_puts(line_buf);
            mac_puts("\r\n");

            if (str_eq(line_buf, expected)) {
                mac_puts("MATCH!\r\n");
            }

            line_len = 0;
            continue;
        }

        line_buf[line_len++] = (char)c;
    }
}
