/**
 * Minimal startup / vector table for STM32F446RE
 *
 * Provides the initial stack pointer, Reset_Handler, and a
 * bare-bones vector table so the chip boots into main().
 */

#include <stdint.h>

/* Symbols defined by the linker script */
extern uint32_t _estack;
extern uint32_t _sdata, _edata, _sidata;
extern uint32_t _sbss, _ebss;

/* main() lives in main.c */
extern int main(void);

void Reset_Handler(void)
{
    /* Copy .data from flash to SRAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata)
        *dst++ = *src++;

    /* Zero-fill .bss */
    dst = &_sbss;
    while (dst < &_ebss)
        *dst++ = 0;

    main();

    /* Trap if main ever returns */
    while (1)
        ;
}

void Default_Handler(void)
{
    while (1)
        ;
}

/* Weak aliases — override in your code if needed */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/* Vector table — placed at 0x0000_0000 in flash by the linker script */
__attribute__((section(".isr_vector")))
uint32_t *vector_table[] = {
    (uint32_t *)&_estack,           /* Initial SP            */
    (uint32_t *)Reset_Handler,      /* Reset                 */
    (uint32_t *)NMI_Handler,        /* NMI                   */
    (uint32_t *)HardFault_Handler,  /* Hard Fault            */
    (uint32_t *)MemManage_Handler,  /* Mem Manage            */
    (uint32_t *)BusFault_Handler,   /* Bus Fault             */
    (uint32_t *)UsageFault_Handler, /* Usage Fault           */
    0, 0, 0, 0,                     /* Reserved              */
    (uint32_t *)SVC_Handler,        /* SVCall                */
    0, 0,                           /* Reserved              */
    (uint32_t *)PendSV_Handler,     /* PendSV                */
    (uint32_t *)SysTick_Handler,    /* SysTick               */
};
