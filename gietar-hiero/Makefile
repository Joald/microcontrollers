# This is the base file to copy when starting new project

PROJ_NAME := gietar_hiero

CC = arm-eabi-gcc
OBJCOPY = arm-eabi-objcopy
FLAGS = -mthumb -mcpu=cortex-m4
CPPFLAGS = -DSTM32F411xE
CFLAGS = $(FLAGS) \
    -std=c2x \
    -DDEBUG \
    -Wall -Wextra -Wpedantic -Wshadow -Wunused \
    -Wcast-align \
    -Wcast-qual -Wno-error=unused-variable \
    -Wdisabled-optimization \
    -Wfloat-equal -Wformat=2 \
    -Wformat-nonliteral -Wformat-security  \
    -Wformat-y2k \
    -Wimport  -Winit-self  -Winline \
    -Winvalid-pch  -Wno-vla \
    -Wlong-long \
    -Wmissing-field-initializers -Wmissing-format-attribute \
    -Wmissing-include-dirs -Wmissing-noreturn \
    -Wpacked -Wpointer-arith \
    -Wredundant-decls \
    -Wshadow \
    -Wstrict-aliasing=2 -Wswitch-default \
    -Wswitch-enum \
    -Wunreachable-code -Wunused \
    -Wunused-parameter \
    -Wvariadic-macros \
    -Wwrite-strings -g \
    -O3 -ffunction-sections -fdata-sections \
    -I/opt/arm/stm32/inc \
    -I/opt/arm/stm32/CMSIS/Include \
    -I/opt/arm/stm32/CMSIS/Device/ST/STM32F4xx/Include \
    -iquote ../lib/include
LDFLAGS = $(FLAGS) -Wl,--gc-sections -nostartfiles \
    -L/opt/arm/stm32/lds -Tstm32f411re.lds \

vpath %.c ../lib/src
vpath %.c /opt/arm/stm32/src


FW_OBJ := startup_stm32.o delay.o gpio.o fonts.o

LIB_SRC_DIR = ../lib/src
# LIB_SRC := $(wildcard $(LIB_SRC_DIR)/*.c)
LIB_SRC := $(LIB_SRC_DIR)/lcd.c $(LIB_SRC_DIR)/keyboard.c $(LIB_SRC_DIR)/dma_uart.c
LIB_OBJ := $(LIB_SRC:$(LIB_SRC_DIR)/%.c=%.o)

OBJECTS = $(PROJ_NAME)_main.o $(LIB_OBJ) $(FW_OBJ)
TARGET = $(PROJ_NAME)

.SECONDARY: $(TARGET).elf $(OBJECTS)
all: $(TARGET).bin
%.elf : $(OBJECTS)
	$(CC) $(LDFLAGS) $^ -o $@
%.bin : %.elf
	$(OBJCOPY) $< $@ -O binary
clean :
	rm -f *.bin *.elf *.hex *.d *.o *.bak *~
