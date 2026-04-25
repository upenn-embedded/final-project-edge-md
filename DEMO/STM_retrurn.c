#include "stm32f4xx.h"

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

#define BUF_SIZE 61440

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

int
main(void) {

    SystemClock_Config_Bare();

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN |

                    RCC_AHB1ENR_GPIOBEN |

                    RCC_AHB1ENR_GPIOCEN;

    // PA5 = LED

    GPIOA->MODER |= (1 << (5 * 2));

    GPIOA->ODR &= ~(1 << 5);

    // I2S3: PA4=WS, PC10=CK, PC12=SD (AF6)

    GPIOA->MODER |= (2 << (4 * 2));

    GPIOA->AFR[0] |= (6 << (4 * 4));

    GPIOC->MODER |= (2 << (10 * 2)) | (2 << (12 * 2));

    GPIOC->AFR[1] |= (6 << ((10 - 8) * 4)) | (6 << ((12 - 8) * 4));

    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    SPI3->I2SCFGR &= ~SPI_I2SCFGR_I2SE;

    SPI3->I2SPR = 93 | SPI_I2SPR_ODD;

    SPI3->I2SCFGR = SPI_I2SCFGR_I2SMOD | (2 << SPI_I2SCFGR_I2SCFG_Pos);

    SPI3->I2SCFGR |= SPI_I2SCFGR_I2SE;

    // SPI2 slave 16-bit: PB12=CS, PB13=CLK, PB15=MOSI (AF5)

    GPIOB->MODER |= (2 << (12 * 2)) | (2 << (13 * 2)) | (2 << (15 * 2));

    GPIOB->AFR[1] |= (5 << ((12 - 8) * 4)) | (5 << ((13 - 8) * 4)) | (5 << ((15 - 8) * 4));

    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

    // 16-bit mode: set DFF bit

    SPI2->CR1 = SPI_CR1_DFF | SPI_CR1_SPE;

    uint32_t count = 0;

    uint8_t started = 0;

    uint8_t send_right = 0;

    uint16_t current_sample = 0;

    while (1) {

        // ── SPI receive 16-bit → buffer ──

        if ((SPI2->SR & SPI_SR_RXNE) && !buf_full()) {

            uint16_t sample = SPI2->DR;   // read 16 bits directly

            if (++count >= 1600) {

                GPIOA->ODR ^= (1 << 5);

                count = 0;
            }

            if (!buf_full())
                buf_push(sample);
        }

        // start playback when buffer 75% full

        uint32_t level = (buf_head - buf_tail + BUF_SIZE) % BUF_SIZE;

        if (!started && level >= (BUF_SIZE * 9 / 10))
            started = 1;

        // ── buffer → I2S ──

        if (started && (SPI3->SR & SPI_SR_TXE)) {

            if (!send_right) {

                current_sample = !buf_empty() ? buf_pop() : 0;

                SPI3->DR = current_sample;

                send_right = 1;

            } else {

                SPI3->DR = current_sample;

                send_right = 0;
            }
        }
    }
}