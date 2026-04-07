/**
 * LED Blink Test — STM32F446RE Nucleo 64
 *
 * Toggles the on board user LED (LD2, PA5) at ~500 ms intervals.
 * No HAL, no libraries — pure register level code so you can
 * verify your toolchain and board are working.
 */

#include <stdint.h>

/*      Base addresses      */
#define RCC_BASE    0x40023800UL
#define GPIOA_BASE  0x40020000UL

/*      RCC registers      */
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))

/*      GPIOA registers      */
#define GPIOA_MODER (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_ODR   (*(volatile uint32_t *)(GPIOA_BASE + 0x14))

/*      Bit positions      */
#define GPIOAEN     (1U << 0)   /* RCC AHB1ENR bit 0  */
#define LED_PIN     5           /* PA5 = LD2           */

static void delay_ms(uint32_t ms)
{
    /* Rough busy wait; assumes ~16 MHz default HSI clock.
       Good enough for a blink test — not cycle accurate. */
    for (volatile uint32_t i = 0; i < ms * 2000; i++)
        ;
}

int main(void)
{
    /* 1. Enable GPIOA clock */
    RCC_AHB1ENR |= GPIOAEN;

    /* 2. Set PA5 as general purpose output (MODER bits [11:10] = 01) */
    GPIOA_MODER &= ~(3U << (LED_PIN * 2));   /* clear */
    GPIOA_MODER |=  (1U << (LED_PIN * 2));    /* set output */

    /* 3. Blink forever */
    while (1) {
        GPIOA_ODR ^= (1U << LED_PIN);  /* toggle PA5 */
        delay_ms(500);
    }
}
