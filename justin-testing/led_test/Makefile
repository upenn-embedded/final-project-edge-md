# Makefile — LED blink test for STM32F446RE Nucleo-64
# Requires: arm-none-eabi-gcc toolchain + st-flash (or OpenOCD)

PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
SIZE    = $(PREFIX)size

TARGET  = led_test

# MCU flags
CPU     = -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS  = $(CPU) -Wall -O2 -std=c11 -ffreestanding -nostdlib
LDFLAGS = $(CPU) -T stm32f446re.ld -nostdlib -Wl,--gc-sections

SRCS    = main.c startup.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET).bin
	$(SIZE) $(TARGET).elf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

$(TARGET).bin: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@

# Flash via st-flash (STLink)
flash: $(TARGET).bin
	st-flash write $(TARGET).bin 0x08000000

# Flash via OpenOCD (alternative)
flash-ocd: $(TARGET).bin
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(TARGET).elf verify reset exit"

clean:
	rm -f $(OBJS) $(TARGET).elf $(TARGET).bin

.PHONY: all flash flash-ocd clean
