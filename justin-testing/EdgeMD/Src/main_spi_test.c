/**
 ******************************************************************************
 * @file    main_spi_test.c
 * @brief   SPI1 loopback test for STM32F411RE (bare-metal, no HAL)
 *
 * PURPOSE:
 *   Verify that SPI1 is wired and configured correctly by sending known bytes
 *   and reading them back through a MOSI→MISO jumper wire.
 *
 * WIRING (the only physical connection you need):
 *   PA7 (MOSI, CN10 pin 15) ──jumper wire──► PA6 (MISO, CN10 pin 13)
 *   No other external hardware required.
 *
 * OUTPUT:
 *   UART2 at 115200 baud on PA2 (same USB/serial port the Nucleo exposes
 *   through ST-Link). Open any terminal at 115200, 8N1.
 *
 * SPI1 pins used (all on GPIOA, Alternate Function 5):
 *   PA5 = SPI1_SCK   (CN10 pin 11)
 *   PA6 = SPI1_MISO  (CN10 pin 13)  ← tie this to PA7
 *   PA7 = SPI1_MOSI  (CN10 pin 15)  ← tie this to PA6
 ******************************************************************************
 */

#include <stdint.h>

/* ── Peripheral base addresses ─────────────────────────────────────────────*/
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)

/* ── RCC ───────────────────────────────────────────────────────────────────*/
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30UL))
#define RCC_APB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x40UL))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44UL))

#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_APB1ENR_USART2EN (1UL << 17)
#define RCC_APB2ENR_SPI1EN  (1UL << 12)

/* ── GPIOA ─────────────────────────────────────────────────────────────────*/
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00UL))
#define GPIOA_AFRL          (*(volatile uint32_t *)(GPIOA_BASE + 0x20UL))

/* ── USART2 (PA2 = TX, AF7) ────────────────────────────────────────────────*/
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)
#define USART2_SR           (*(volatile uint32_t *)(USART2_BASE + 0x00UL))
#define USART2_DR           (*(volatile uint32_t *)(USART2_BASE + 0x04UL))
#define USART2_BRR          (*(volatile uint32_t *)(USART2_BASE + 0x08UL))
#define USART2_CR1          (*(volatile uint32_t *)(USART2_BASE + 0x0CUL))

/* ── SPI1 (PA5=SCK, PA6=MISO, PA7=MOSI, AF5) ──────────────────────────────*/
#define SPI1_BASE           (APB2PERIPH_BASE + 0x3000UL)
#define SPI1_CR1            (*(volatile uint32_t *)(SPI1_BASE + 0x00UL))
#define SPI1_CR2            (*(volatile uint32_t *)(SPI1_BASE + 0x04UL))
#define SPI1_SR             (*(volatile uint32_t *)(SPI1_BASE + 0x08UL))
#define SPI1_DR             (*(volatile uint32_t *)(SPI1_BASE + 0x0CUL))

/* SPI1_SR flags */
#define SPI_SR_RXNE         (1UL << 0)   /* RX buffer not empty */
#define SPI_SR_TXE          (1UL << 1)   /* TX buffer empty     */
#define SPI_SR_BSY          (1UL << 7)   /* busy flag           */

/* SPI1_CR1 bits */
#define SPI_CR1_CPHA        (1UL << 0)
#define SPI_CR1_CPOL        (1UL << 1)
#define SPI_CR1_MSTR        (1UL << 2)   /* master mode         */
#define SPI_CR1_BR_DIV16    (3UL << 3)   /* fPCLK/16 = 1 MHz   */
#define SPI_CR1_SPE         (1UL << 6)   /* SPI enable          */
#define SPI_CR1_SSI         (1UL << 8)   /* internal NSS high   */
#define SPI_CR1_SSM         (1UL << 9)   /* software NSS mgmt   */

/* ── UART helpers ──────────────────────────────────────────────────────────*/

static void uart_init(void)
{
    /* PA2 → AF7 (USART2_TX), output */
    GPIOA_MODER &= ~(3UL << (2 * 2));
    GPIOA_MODER |=  (2UL << (2 * 2));         /* alternate function */
    GPIOA_AFRL  &= ~(0xFUL << (2 * 4));
    GPIOA_AFRL  |=  (7UL   << (2 * 4));       /* AF7 = USART2       */

    /* 115200 baud @ 16 MHz HSI: USARTDIV = 16e6/115200 ≈ 139 = 0x8B */
    USART2_BRR = 0x8BUL;
    USART2_CR1 = (1UL << 13) | (1UL << 3);    /* UE + TE            */
}

static void uart_putc(char c)
{
    while (!(USART2_SR & (1UL << 7))) {}       /* wait TXE           */
    USART2_DR = (uint32_t)c;
}

static void uart_puts(const char *s)
{
    while (*s) uart_putc(*s++);
}

/* Print a byte as "0xXX" */
static void uart_put_hex(uint8_t b)
{
    const char hex[] = "0123456789ABCDEF";
    uart_putc('0');
    uart_putc('x');
    uart_putc(hex[(b >> 4) & 0xF]);
    uart_putc(hex[ b       & 0xF]);
}

/* Print a decimal uint32 */
static void uart_put_dec(uint32_t n)
{
    char buf[11];
    int i = 0;
    if (n == 0) { uart_putc('0'); return; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i > 0) uart_putc(buf[--i]);
}

/* ── SPI1 init ─────────────────────────────────────────────────────────────*/

static void spi1_init(void)
{
    /* Configure PA5, PA6, PA7 as AF5 (SPI1: SCK, MISO, MOSI) */
    /* MODER bits: 10 = alternate function */
    GPIOA_MODER &= ~((3UL << (5 * 2)) | (3UL << (6 * 2)) | (3UL << (7 * 2)));
    GPIOA_MODER |=   (2UL << (5 * 2)) | (2UL << (6 * 2)) | (2UL << (7 * 2));

    /* AFRL: AF5 for PA5, PA6, PA7 (AFRL covers PA0–PA7, 4 bits each) */
    GPIOA_AFRL &= ~((0xFUL << (5 * 4)) | (0xFUL << (6 * 4)) | (0xFUL << (7 * 4)));
    GPIOA_AFRL |=   (5UL   << (5 * 4)) | (5UL   << (6 * 4)) | (5UL   << (7 * 4));

    /* SPI1 config: master, Mode 0 (CPOL=0, CPHA=0), 8-bit, fPCLK/16,
     * software NSS (SSM=1, SSI=1 keeps NSS line logically high) */
    SPI1_CR1 = 0;
    SPI1_CR2 = 0;
    SPI1_CR1 = SPI_CR1_MSTR
             | SPI_CR1_BR_DIV16
             | SPI_CR1_SSM
             | SPI_CR1_SSI;

    /* Enable SPI1 */
    SPI1_CR1 |= SPI_CR1_SPE;
}

/* ── SPI transmit + receive one byte ───────────────────────────────────────*/

static uint8_t spi1_transfer(uint8_t tx)
{
    /* Wait until TX buffer is empty */
    while (!(SPI1_SR & SPI_SR_TXE)) {}

    /* Load the byte to transmit */
    SPI1_DR = tx;

    /* Wait until a byte has been received (RXNE) */
    while (!(SPI1_SR & SPI_SR_RXNE)) {}

    /* Wait for BSY to clear (transfer fully complete) */
    while (SPI1_SR & SPI_SR_BSY) {}

    /* Return the received byte */
    return (uint8_t)(SPI1_DR & 0xFFUL);
}

/* ── Test cases ────────────────────────────────────────────────────────────*/

static const uint8_t test_bytes[] = {
    0xA5, 0x5A, 0xFF, 0x00, 0x01, 0x80, 0xF0, 0x0F
};
#define NUM_TESTS (sizeof(test_bytes) / sizeof(test_bytes[0]))

/* ── Main ──────────────────────────────────────────────────────────────────*/

int main(void)
{
    /* 1. Enable clocks: GPIOA, USART2 (APB1), SPI1 (APB2) */
    RCC_AHB1ENR  |= RCC_AHB1ENR_GPIOAEN;
    RCC_APB1ENR  |= RCC_APB1ENR_USART2EN;
    RCC_APB2ENR  |= RCC_APB2ENR_SPI1EN;

    /* 2. Init UART for debug output */
    uart_init();

    uart_puts("\r\n=============================\r\n");
    uart_puts("  SPI1 Loopback Test\r\n");
    uart_puts("  STM32F411RE — EDGE MD\r\n");
    uart_puts("=============================\r\n");
    uart_puts("Make sure PA7 (MOSI) is wired to PA6 (MISO)!\r\n\r\n");

    /* 3. Init SPI1 */
    spi1_init();

    /* 4. Run loopback test */
    uint32_t pass = 0, fail = 0;

    for (uint32_t i = 0; i < NUM_TESTS; i++)
    {
        uint8_t tx = test_bytes[i];
        uint8_t rx = spi1_transfer(tx);

        uart_puts("TX: ");
        uart_put_hex(tx);
        uart_puts("  RX: ");
        uart_put_hex(rx);
        uart_puts("  ");

        if (rx == tx)
        {
            uart_puts("PASS\r\n");
            pass++;
        }
        else
        {
            uart_puts("FAIL  <-- check jumper wire PA7→PA6\r\n");
            fail++;
        }
    }

    uart_puts("\r\n-----------------------------\r\n");
    uart_put_dec(pass);
    uart_puts(" passed, ");
    uart_put_dec(fail);
    uart_puts(" failed out of ");
    uart_put_dec(NUM_TESTS);
    uart_puts(" tests.\r\n");

    if (fail == 0)
        uart_puts("All tests PASSED. SPI1 is working!\r\n");
    else
        uart_puts("Some tests FAILED. Check wiring and config.\r\n");

    uart_puts("=============================\r\n");

    /* 5. Blink at end so you know program is running */
    while (1) {}
}
