#include "stm32f4xx.h"

#include <stdint.h>

/* ═══════════════════════════════════════════════════

 * CONFIGURATION

 * ═══════════════════════════════════════════════════ */

#define SAMPLE_RATE 16000U

#define RECORD_SAMPLES (SAMPLE_RATE * 5)

#define DC_BIAS 2048U

#define BUF_SIZE 61440U

/* Playback tuning knobs

 * PREBUF_THRESHOLD : words buffered before first sound (~20ms at 16kHz stereo)

 * UNDERRUN_GUARD   : minimum words before we stall drain (~5ms headroom)

 * IDLE_TIMEOUT_LOOPS: consecutive empty-SPI loops before declaring stream done

 *                     Tune this to your loop speed. ~80000 ≈ 5–10ms on a tight loop.

 */

#define PREBUF_THRESHOLD 640U

#define UNDERRUN_GUARD 160U

#define IDLE_TIMEOUT_LOOPS 80000U

/* ═══════════════════════════════════════════════════

 * CIRCULAR BUFFER

 * ═══════════════════════════════════════════════════ */

static uint16_t audio_buf[BUF_SIZE];

static volatile uint32_t buf_head = 0;

static volatile uint32_t buf_tail = 0;

static inline int
buf_empty(void) {
    return buf_head == buf_tail;
}

static inline int
buf_full(void) {
    return ((buf_head + 1) % BUF_SIZE) == buf_tail;
}

static inline void
buf_push(uint16_t v) {
    audio_buf[buf_head] = v;
    buf_head = (buf_head + 1) % BUF_SIZE;
}

static inline uint16_t
buf_pop(void) {
    uint16_t v = audio_buf[buf_tail];
    buf_tail = (buf_tail + 1) % BUF_SIZE;
    return v;
}

static inline uint32_t
buf_level(void) {
    return (buf_head - buf_tail + BUF_SIZE) % BUF_SIZE;
}

/* ═══════════════════════════════════════════════════

 * SPI HELPERS

 * ═══════════════════════════════════════════════════ */

static inline void
clear_spi_ovr(void) {

    volatile uint16_t tmp = SPI2->DR;

    volatile uint32_t tmp2 = SPI2->SR;

    (void) tmp;
    (void) tmp2;
}

static void
spi2_flush(void) {

    while (SPI2->SR & SPI_SR_RXNE) {

        volatile uint16_t tmp = SPI2->DR;

        (void) tmp;
    }

    if (SPI2->SR & SPI_SR_OVR)
        clear_spi_ovr();

    buf_head = 0;

    buf_tail = 0;
}

/* ═══════════════════════════════════════════════════

 * CLOCK

 * ═══════════════════════════════════════════════════ */

void
SystemClock_Config_Bare(void) {

    RCC->CR |= RCC_CR_HSION;

    while (!(RCC->CR & RCC_CR_HSIRDY))
        ;

    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_PRFTEN |

                 FLASH_ACR_ICEN | FLASH_ACR_DCEN;

    RCC->PLLCFGR = (16 << 0) | (336 << 6) | (1 << 16);

    RCC->CR |= RCC_CR_PLLON;

    while (!(RCC->CR & RCC_CR_PLLRDY))
        ;

    RCC->PLLI2SCFGR = (16 << 0) | (192 << 6) | (2 << 28);

    RCC->CR |= RCC_CR_PLLI2SON;

    while (!(RCC->CR & RCC_CR_PLLI2SRDY))
        ;

    RCC->CFGR = RCC_CFGR_PPRE1_DIV2;

    RCC->CFGR |= RCC_CFGR_SW_PLL;

    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL)
        ;
}

/* ═══════════════════════════════════════════════════

 * DELAY

 * ═══════════════════════════════════════════════════ */

static void
delay_ms(uint32_t ms) {

    for (volatile uint32_t i = 0; i < ms * 8400; i++)
        ;
}

/* ═══════════════════════════════════════════════════

 * ADC

 * ═══════════════════════════════════════════════════ */

static void
adc_init(void) {

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    GPIOA->MODER |= (3 << (0 * 2));   // PA0 analog

    ADC1->CR1 = 0;

    ADC1->CR2 = 0;

    ADC1->SQR3 = 0;

    ADC1->SQR1 = 0;

    ADC1->SMPR2 = (7 << 0);   // max sample time on CH0

    ADC1->CR2 |= ADC_CR2_ADON;

    delay_ms(1);
}

static uint16_t
adc_read(void) {

    ADC1->CR2 |= ADC_CR2_SWSTART;

    while (!(ADC1->SR & ADC_SR_EOC))
        ;

    return (uint16_t) ADC1->DR;
}

/* ═══════════════════════════════════════════════════

 * USART1 TX (to Raspberry Pi)

 * ═══════════════════════════════════════════════════ */

static void
usart1_tx_init(void) {

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

    GPIOA->MODER |= (2 << (9 * 2));

    GPIOA->AFR[1] |= (7 << ((9 - 8) * 4));

    USART1->BRR = 84000000 / 921600;

    USART1->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void
usart1_send_byte(uint8_t b) {

    while (!(USART1->SR & USART_SR_TXE))
        ;

    USART1->DR = b;
}

/* ═══════════════════════════════════════════════════

 * I2S3 (speaker output)

 * ═══════════════════════════════════════════════════ */

static void
i2s3_init(void) {

    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;

    SPI3->I2SPR = 93 | SPI_I2SPR_ODD;

    SPI3->I2SCFGR = SPI_I2SCFGR_I2SMOD | (2 << SPI_I2SCFGR_I2SCFG_Pos);

    SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;
}

/* ═══════════════════════════════════════════════════

 * SPI2 SLAVE (audio in from Raspberry Pi)

 * ═══════════════════════════════════════════════════ */

static void
spi2_slave_init(void) {

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    SPI2->CR1 = SPI_CR1_DFF | SPI_CR1_SPE;
}

/* ═══════════════════════════════════════════════════

 * GPIO

 * ═══════════════════════════════════════════════════ */

static void
gpio_init(void) {

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |

                    RCC_AHB1ENR_GPIOBEN |

                    RCC_AHB1ENR_GPIOCEN;

    // PA5 = debug LED

    GPIOA->MODER |= (1 << (5 * 2));

    GPIOA->ODR &= ~(1 << 5);

    // I2S3: PA4=WS, PC10=CK, PC12=SD (AF6)

    GPIOA->MODER |= (2 << (4 * 2));

    GPIOA->AFR[0] |= (6 << (4 * 4));

    GPIOC->MODER |= (2 << (10 * 2)) | (2 << (12 * 2));

    GPIOC->AFR[1] |= (6 << ((10 - 8) * 4)) | (6 << ((12 - 8) * 4));

    // SPI2: PB12=CS, PB13=CLK, PB15=MOSI (AF5)

    GPIOB->MODER |= (2 << (12 * 2)) | (2 << (13 * 2)) | (2 << (15 * 2));

    GPIOB->AFR[1] |= (5 << ((12 - 8) * 4)) | (5 << ((13 - 8) * 4)) | (5 << ((15 - 8) * 4));
}

/* ═══════════════════════════════════════════════════

 * MAIN

 * ═══════════════════════════════════════════════════ */

int
main(void) {

    SystemClock_Config_Bare();

    gpio_init();

    adc_init();

    usart1_tx_init();

    i2s3_init();

    spi2_slave_init();

    while (1) {

        /* ════════════════════════════════════════

         * PHASE 0: RECORD (5 seconds)

         * ════════════════════════════════════════ */

        GPIOA->ODR |= (1 << 5);   // LED ON = recording

        spi2_flush();

        // Send START marker to RPi

        usart1_send_byte(0xAA);

        usart1_send_byte(0xBB);

        uint32_t samples_recorded = 0;

        while (samples_recorded < RECORD_SAMPLES) {

            for (volatile uint32_t d = 0; d < 5250; d++)
                ;

            uint16_t raw = adc_read();

            int32_t s = ((int32_t) raw - DC_BIAS) * 16;

            if (s > 32767)
                s = 32767;

            if (s < -32768)
                s = -32768;

            int16_t pcm = (int16_t) s;

            usart1_send_byte((uint8_t) (pcm & 0xFF));

            usart1_send_byte((uint8_t) ((pcm >> 8) & 0xFF));

            samples_recorded++;
        }

        // Send END marker to RPi

        usart1_send_byte(0xFF);

        usart1_send_byte(0xFE);

        GPIOA->ODR &= ~(1 << 5);   // LED OFF = done recording, waiting for RPi

        /* ════════════════════════════════════════

         * PHASE 1: PLAYBACK

         *

         * Design goals:

         *   - Ultra-low latency: start draining after only ~20ms of pre-fill

         *   - Underrun protection: stall I2S drain if buffer goes dangerously low,

         *     then resume once refilled — avoids clicks/pops without hard stopping

         *   - OVR resilience: SPI RX is burst-drained every loop iteration so the

         *     hardware FIFO never overflows

         *   - Natural end: exits when RPi goes silent AND buffer is fully drained

         *   - Debug LED: blinks on live SPI data activity (preserved)

         * ════════════════════════════════════════ */

        spi2_flush();

        uint8_t playback_started = 0;   // gate: true once pre-fill threshold hit

        uint8_t underrun_stall = 0;   // true while recovering from near-empty buffer

        uint8_t spi_done = 0;   // true once RPi has gone idle

        uint32_t led_count = 0;   // LED blink counter (debug)

        uint32_t idle_loops = 0;   // consecutive loops with no SPI data

        while (1) {

            /* ── 1. Burst-drain SPI RX FIFO ──────────────────────────────

             * Pull every available word in a tight inner loop.

             * This is the most critical step — we must never let OVR set.

             * If the buffer is full, we drop the incoming word (not the oldest)

             * to preserve already-queued audio that is currently playing.

             * If your RPi sends faster than the I2S drains, increase BUF_SIZE. */

            uint8_t got_spi = 0;

            while (SPI2->SR & SPI_SR_RXNE) {

                uint16_t sample = SPI2->DR;

                got_spi = 1;

                if (!buf_full()) {

                    buf_push(sample);
                }

                // Debug LED: blink every 1600 words received (~100ms of stereo audio)

                if (++led_count >= 1600) {

                    GPIOA->ODR ^= (1 << 5);

                    led_count = 0;
                }
            }

            /* ── 2. Clear OVR immediately after drain ─────────────────────

             * Must happen right after reading DR, not later, or SR is stale. */

            if (SPI2->SR & SPI_SR_OVR)
                clear_spi_ovr();

            /* ── 3. End-of-stream detection ───────────────────────────────

             * Count consecutive loops with no SPI activity.

             * Once the RPi finishes sending, idle_loops will exceed the

             * threshold and we mark spi_done to begin final drain. */

            if (!spi_done) {

                if (got_spi) {

                    idle_loops = 0;

                } else {

                    if (++idle_loops >= IDLE_TIMEOUT_LOOPS) {

                        spi_done = 1;
                    }
                }
            }

            /* ── 4. Pre-fill gate ─────────────────────────────────────────

             * Don't start draining until we have at least PREBUF_THRESHOLD

             * words. This gives the I2S peripheral a comfortable runway so

             * the very first drain doesn't immediately starve. */

            uint32_t level = buf_level();

            if (!playback_started && level >= PREBUF_THRESHOLD) {

                playback_started = 1;

                underrun_stall = 0;
            }

            /* ── 5. Underrun hysteresis ───────────────────────────────────

             * If the buffer drops below UNDERRUN_GUARD while the RPi is

             * still sending, stall the I2S drain until the buffer refills

             * back to PREBUF_THRESHOLD. This prevents the characteristic

             * click/pop of a starved I2S stream.

             * Once spi_done is set, skip this — just drain everything out. */

            if (playback_started && !spi_done) {

                if (underrun_stall) {

                    if (level >= PREBUF_THRESHOLD)
                        underrun_stall = 0;   // recovered

                } else {

                    if (level < UNDERRUN_GUARD)
                        underrun_stall = 1;   // too low
                }
            }

            /* ── 6. Feed I2S TX ───────────────────────────────────────────

             * One word per outer loop iteration. TXE check ensures we never

             * write to a full TX register. Conditions to drain:

             *   - playback has started (pre-fill gate passed)

             *   - not stalled for underrun recovery

             *   - buffer is not empty

             *   - I2S TX register is ready */

            if (playback_started &&

                !underrun_stall &&

                !buf_empty() &&

                (SPI3->SR & SPI_SR_TXE)) {

                SPI3->DR = buf_pop();
            }

            /* ── 7. Exit ──────────────────────────────────────────────────

             * Only exit once the RPi has gone idle AND the buffer is fully

             * drained. This guarantees we never cut off the tail of audio. */

            if (spi_done && buf_empty())
                break;
        }

        /* Playback complete — clean up LED and pause before next cycle */

        GPIOA->ODR &= ~(1 << 5);

        delay_ms(200);
    }
}