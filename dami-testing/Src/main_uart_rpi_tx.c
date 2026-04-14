/**
 ******************************************************************************
 * @file           : main_uart_rpi_tx.c
 * @brief          : Continuously send "hola como estas\n" from STM32 to RPi5.
 *
 *   USART6 (RPi link): PA11 = TX  --> RPi5 RX (GPIO15 / header pin 10)
 *                      PA12 = RX  <-- RPi5 TX (GPIO14 / header pin 8)
 *                      GND  <->   GND
 *                      USART6 uses AF8 on PA11/PA12.
 *
 *   USART2 (Mac debug via ST-Link VCP):
 *                      PA2  = TX  --> Mac terminal
 *
 *   Both at 115200 8N1. STM32 runs on 16 MHz HSI after reset, so
 *   USARTDIV = 16e6 / (16 * 115200) ~= 8.68 -> BRR = 0x8B for both.
 *
 *   Behavior: in an infinite loop, transmit "hola como estas\n" over USART6
 *   to the Pi, then print "SENT" on USART2 so you can see progress in your
 *   Mac terminal, then busy-wait ~1 second and repeat.
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
#define USART6_DR     (*(volatile uint32_t *)0x40011404)
#define USART6_BRR    (*(volatile uint32_t *)0x40011408)
#define USART6_CR1    (*(volatile uint32_t *)0x4001140C)

static const char message[] = "hola como estas\n";

/* ---------- USART2 TX (to Mac) ---------- */
static void mac_putc(char c) {
    while (!(USART2_SR & (1 << 7))) { }   /* wait TXE */
    USART2_DR = (uint8_t)c;
}

static void mac_puts(const char *s) {
    while (*s) mac_putc(*s++);
}

/* ---------- USART6 TX (to RPi) ---------- */
static void rpi_putc(char c) {
    while (!(USART6_SR & (1 << 7))) { }   /* wait TXE */
    USART6_DR = (uint8_t)c;
}

static void rpi_puts(const char *s) {
    while (*s) rpi_putc(*s++);
}

/* Crude busy-wait. ~5M iterations ~= 1 s at 16 MHz HSI. */
static void delay(volatile uint32_t n) {
    while (n--) {
        __asm__ volatile ("nop");
    }
}

int main(void) {
    /* Clocks: GPIOA, USART6 (APB2), USART2 (APB1) */
    RCC_AHB1ENR |= (1 << 0);          /* GPIOAEN  */
    RCC_APB2ENR |= (1 << 5);          /* USART6EN (APB2 bit 5) */
    RCC_APB1ENR |= (1 << 17);         /* USART2EN */

    /* PA2 = USART2 TX (AF7) */
    GPIOA_MODER &= ~(3 << (2 * 2));
    GPIOA_MODER |=  (2 << (2 * 2));
    GPIOA_AFRL  &= ~(0xF << (2 * 4));
    GPIOA_AFRL  |=  (7   << (2 * 4));

    /* PA11 = USART6 TX (AF8) -- the wire that actually carries data to the Pi */
    GPIOA_MODER &= ~(3 << (11 * 2));
    GPIOA_MODER |=  (2 << (11 * 2));
    GPIOA_AFRH  &= ~(0xF << ((11 - 8) * 4));
    GPIOA_AFRH  |=  (8   << ((11 - 8) * 4));

    /* PA12 = USART6 RX (AF8) -- configured but unused for now */
    GPIOA_MODER &= ~(3 << (12 * 2));
    GPIOA_MODER |=  (2 << (12 * 2));
    GPIOA_AFRH  &= ~(0xF << ((12 - 8) * 4));
    GPIOA_AFRH  |=  (8   << ((12 - 8) * 4));

    /* 115200 baud @ 16 MHz HSI: BRR = 0x8B for both */
    USART2_BRR = 0x8B;
    USART2_CR1 = (1 << 13) | (1 << 3);    /* UE + TE */

    USART6_BRR = 0x8B;
    USART6_CR1 = (1 << 13) | (1 << 3);    /* UE + TE */

    mac_puts("\r\n--- RPi UART TX ready, sending \"hola como estas\" forever ---\r\n");

    uint32_t count = 0;
    while (1) {
        rpi_puts(message);

        mac_puts("SENT #");
        /* tiny integer print, base 10, no leading zeros */
        char digits[11];
        int di = 0;
        uint32_t v = ++count;
        if (v == 0) {
            digits[di++] = '0';
        } else {
            while (v > 0) { digits[di++] = (char)('0' + (v % 10)); v /= 10; }
        }
        while (di > 0) mac_putc(digits[--di]);
        mac_puts("\r\n");

        delay(5000000);  /* ~1 s-ish at 16 MHz */
    }
}
