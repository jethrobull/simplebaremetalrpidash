# Raspberry Pi 3 Bare-Metal (AArch64) — gs_usb CANable Project
# Produces kernel8.img

CROSS   = aarch64-none-elf-
CC      = $(CROSS)gcc
LD      = $(CROSS)ld
AS      = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy

CFLAGS  = -Wall -O2 -ffreestanding -nostdlib -nostartfiles -mgeneral-regs-only -march=armv8-a
CFLAGS += -Iinclude

LDFLAGS = -T linker.ld -nostdlib

SRC = \
    start.S \
    src/gpio.c \
    src/uart.c \
    src/timer.c \
    src/gic.c \
    src/mailbox.c \
    src/usb_core.c \
    src/usb_dwc2.c \
    src/gauges.c \
    src/font8x12.c \
    src/framebuffer.c \
    src/mcp2515.c \
    src/spio.c \
    src/main.c

OBJ = $(SRC:.c=.o)
OBJ := $(OBJ:.S=.o)

all: kernel8.img

kernel8.img: kernel8.elf
	$(OBJCOPY) kernel8.elf -O binary kernel8.img

kernel8.elf: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ)

clean:
	rm -f $(OBJ) kernel8.elf kernel8.img

%.o: %.S
	$(AS) -x assembler-with-cpp -march=armv8-a $(CFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: all clean
