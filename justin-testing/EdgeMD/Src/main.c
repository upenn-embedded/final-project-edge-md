/**
 * NUCLEO-F411RE — Battery voltage monitor (bare metal, no HAL)
 *
 * Reads voltage via ADC1 on PA0 (A0 / CN8 pin 1).
 * Drives blue LED on PB0 HIGH when voltage drops below threshold.
 *
 * Wiring:
 *   Power supply (+) → R1 (10kΩ) → PA0 → R2 (10kΩ) → GND
 *   Blue LED anode   → PB0 → 220Ω → GND
 *
 * Voltage divider: ADC sees Vsupply / 2
 * Threshold: 3.0V supply → 1.5V on ADC
 * ADC raw:   (1.5 / 3.3) * 4095 = 1861
 *
 * Polling interval: 100 ms (fast enough for battery monitoring)
 */

#include <stdint.h>

/* ── Register bases ── */
#define PERIPH_BASE         0x40000000UL
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)

#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800UL)
#define ADC1_BASE           (APB2PERIPH_BASE + 0x2000UL)
#define ADC_COMMON_BASE     (APB2PERIPH_BASE + 0x2300UL)

/* ── RCC ── */
#define RCC_AHB1ENR         (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_APB2ENR         (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_AHB1ENR_GPIOAEN (1UL << 0)
#define RCC_AHB1ENR_GPIOBEN (1UL << 1)
#define RCC_AHB1ENR_GPIOCEN (1UL << 2)
#define RCC_APB2ENR_ADC1EN  (1UL << 8)

/* ── GPIOA ── */
#define GPIOA_MODER         (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_PUPDR         (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))

/* ── GPIOB (PB0 = blue LED) ── */
#define GPIOB_MODER         (*(volatile uint32_t *)(GPIOB_BASE + 0x00))
#define GPIOB_OTYPER        (*(volatile uint32_t *)(GPIOB_BASE + 0x04))
#define GPIOB_OSPEEDR       (*(volatile uint32_t *)(GPIOB_BASE + 0x08))
#define GPIOB_PUPDR         (*(volatile uint32_t *)(GPIOB_BASE + 0x0C))
#define GPIOB_BSRR          (*(volatile uint32_t *)(GPIOB_BASE + 0x18))

/* ── GPIOC (PC13 = B1 button, kept from blink) ── */
#define GPIOC_MODER         (*(volatile uint32_t *)(GPIOC_BASE + 0x00))
#define GPIOC_PUPDR         (*(volatile uint32_t *)(GPIOC_BASE + 0x0C))
#define GPIOC_IDR           (*(volatile uint32_t *)(GPIOC_BASE + 0x10))

/* ── ADC1 ── */
#define ADC1_SR             (*(volatile uint32_t *)(ADC1_BASE + 0x00))
#define ADC1_CR1            (*(volatile uint32_t *)(ADC1_BASE + 0x04))
#define ADC1_CR2            (*(volatile uint32_t *)(ADC1_BASE + 0x08))
#define ADC1_SMPR2          (*(volatile uint32_t *)(ADC1_BASE + 0x14))
#define ADC1_SQR1           (*(volatile uint32_t *)(ADC1_BASE + 0x2C))
#define ADC1_SQR3           (*(volatile uint32_t *)(ADC1_BASE + 0x34))
#define ADC1_DR             (*(volatile uint32_t *)(ADC1_BASE + 0x4C))

/* ADC common: prescaler */
#define ADC_CCR             (*(volatile uint32_t *)(ADC_COMMON_BASE + 0x04))

/* ── SysTick ── */
#define SYSTICK_BASE        0xE000E010UL
#define SYSTICK_CTRL        (*(volatile uint32_t *)(SYSTICK_BASE + 0x00))
#define SYSTICK_LOAD        (*(volatile uint32_t *)(SYSTICK_BASE + 0x04))
#define SYSTICK_VAL         (*(volatile uint32_t *)(SYSTICK_BASE + 0x08))

/* ── Constants ── */
#define BATT_THRESHOLD_RAW  1861u   /* 1.5V on ADC = 3.0V on supply (÷2 divider) */
#define POLL_INTERVAL_MS    100u    /* check every 100 ms                         */

/* ── SysTick ── */
static volatile uint32_t g_tick = 0;

void SysTick_Handler(void)
{
    g_tick++;
}

static uint32_t get_tick(void) { return g_tick; }

static void delay_ms(uint32_t ms)
{
    uint32_t start = get_tick();
    while ((get_tick() - start) < ms) {}
}

/* ── ADC init ── */
static void adc_init(void)
{
    /* 1. Enable ADC1 clock on APB2 */
    RCC_APB2ENR |= RCC_APB2ENR_ADC1EN;

    /* 2. PA0 → analog mode (MODER bits [1:0] = 11) */
    GPIOA_MODER |= (0x3UL << (0 * 2));    /* PA0 analog */
    GPIOA_PUPDR &= ~(0x3UL << (0 * 2));   /* no pull    */

    /* 3. ADC prescaler = /4 → 16MHz/4 = 4MHz (must be ≤ 36MHz) */
    ADC_CCR &= ~(0x3UL << 16);
    ADC_CCR |=  (0x1UL << 16);   /* ADCPRE = 01 → /4 */

    /* 4. Resolution = 12-bit (default, CR1 bits [25:24] = 00) */
    ADC1_CR1 &= ~(0x3UL << 24);

    /* 5. Single conversion mode (CR2 CONT = 0) */
    ADC1_CR2 &= ~(1UL << 1);

    /* 6. Sample time for channel 0 = 480 cycles (slowest = most accurate)
     *    SMPR2 bits [2:0] for channel 0 = 111 */
    ADC1_SMPR2 |= (0x7UL << 0);

    /* 7. Sequence: 1 conversion, channel 0
     *    SQR1 L[23:20] = 0000 (1 conversion)
     *    SQR3 SQ1[4:0] = 00000 (channel 0) */
    ADC1_SQR1 &= ~(0xFUL << 20);
    ADC1_SQR3 &= ~(0x1FUL << 0);

    /* 8. Enable ADC */
    ADC1_CR2 |= (1UL << 0);   /* ADON = 1 */

    /* 9. Wait for ADC to stabilize (~10 us) */
    delay_ms(1);
}

/* ── Trigger one conversion, wait, return raw 12-bit result ── */
static uint32_t adc_read(void)
{
    /* Clear EOC flag */
    ADC1_SR &= ~(1UL << 1);

    /* Start conversion (SWSTART) */
    ADC1_CR2 |= (1UL << 30);

    /* Wait for EOC (end of conversion) */
    while (!(ADC1_SR & (1UL << 1))) {}

    return ADC1_DR & 0xFFF;   /* 12-bit result */
}

/* ── Blue LED helpers (PB0) ── */
static void blue_led_on(void)  { GPIOB_BSRR = (1UL << 0);        }
static void blue_led_off(void) { GPIOB_BSRR = (1UL << (0 + 16)); }

int main(void)
{
    /* ── SysTick: 1 ms at 16 MHz HSI ── */
    SYSTICK_LOAD = 16000 - 1;
    SYSTICK_VAL  = 0;
    SYSTICK_CTRL = (1UL << 2) | (1UL << 1) | (1UL << 0);

    /* ── Enable GPIO clocks ── */
    RCC_AHB1ENR |= RCC_AHB1ENR_GPIOAEN
                 | RCC_AHB1ENR_GPIOBEN
                 | RCC_AHB1ENR_GPIOCEN;

    /* ── PB0 → output push-pull (blue LED) ── */
    GPIOB_MODER  &= ~(0x3UL << (0 * 2));
    GPIOB_MODER  |=  (0x1UL << (0 * 2));   /* output */
    GPIOB_OTYPER &= ~(1UL << 0);            /* push-pull */
    GPIOB_OSPEEDR &= ~(0x3UL << (0 * 2));  /* low speed */
    GPIOB_PUPDR  &= ~(0x3UL << (0 * 2));   /* no pull */
    blue_led_off();

    /* ── ADC init (also configures PA0 as analog) ── */
    adc_init();

    /* ── Poll loop ── */
    uint32_t last_poll = 0;

    while (1)
    {
        uint32_t now = get_tick();

        if (now - last_poll >= POLL_INTERVAL_MS)
        {
            last_poll = now;

            uint32_t raw = adc_read();

            /* raw < threshold means voltage dropped below 3V supply */
            if (raw < BATT_THRESHOLD_RAW)
                blue_led_on();
            else
                blue_led_off();
        }
    }
}
