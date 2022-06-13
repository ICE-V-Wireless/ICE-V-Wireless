# Makefile for RISC-V on iCE40UP5k

CROSS = /opt/riscv-gcc/riscv64-unknown-elf-gcc-8.3.0-2019.08.0-x86_64-linux-ubuntu14/bin/riscv64-unknown-elf-
CC = $(CROSS)gcc
OBJCOPY = $(CROSS)objcopy
OBJDUMP = $(CROSS)objdump
ICEPROG = iceprog
HEXDUMP = hexdump
HEXDUMP_ARGS = -v -e '1/4 "%08x" "\n"'

#CFLAGS=-Wall -Os -march=rv32i -mabi=ilp32 -ffreestanding -nostartfiles -flto
CFLAGS=-Wall -Os -march=rv32i -mabi=ilp32 -ffreestanding -flto -nostartfiles -fomit-frame-pointer

HEADER = up5k_osc.h acia.h clkcnt.h printf.h spi.h i2c.h psram.h mailbox.h

SOURCES = start.S main.c acia.c clkcnt.c printf.c spi.c i2c.c psram.c mailbox.c

main.elf: lnk-app.lds $(HEADERS) $(SOURCES)
	$(CC) $(CFLAGS)  -Wl,-Bstatic,-T,lnk-app.lds,--strip-debug,--print-memory-usage -o $@ $(SOURCES)

disassemble: main.elf
	$(OBJDUMP) -d main.elf > main.dis

%.bin: %.elf
	$(OBJCOPY) -O binary $< $@

%.hex: %.bin
	$(HEXDUMP) $(HEXDUMP_ARGS) $< >$@

clean:
	rm -f *.bin *.hex *.elf *.dis
