# Makefile for icestorm tools + yosys + nextpnr
# Modified from examples in icestorm distribution
# 01-16-18 E. Brombaugh

# src directory
VPATH = ../src ../riscv/src

SRC =	../src/up5k_esp32c3.v ../src/spi_slave.v \
		../riscv/src/system.v ../riscv/src/picorv32.v \
		../riscv/src/spram_16kx32.v \
		../riscv/src/acia.v ../riscv/src/acia_tx.v ../riscv/src/acia_rx.v \
		../riscv/src/wb_bus.v ../riscv/src/wb_master.v \
		../riscv/src/mailbox.v ../riscv/src/fifo1.v

# preparing the machine code
FAKE_HEX =	rom.hex
REAL_HEX =  code.hex

# project stuff
PROJ = up5k_esp32c3
PIN_DEF = ../src/up5k_esp32c3.pcf
SDC = ../src/up5k_esp32c3.sdc
DEVICE = up5k
PACKAGE = sg48

TOOLS = /opt/openfpga/fpga-toolchain
YOSYS = $(TOOLS)/bin/yosys
NEXTPNR = $(TOOLS)/bin/nextpnr-ice40
NEXTPNR_ARGS = --pre-pack $(SDC) --package $(PACKAGE) --$(DEVICE)
ICEPACK = $(TOOLS)/bin/icepack
ICETIME = $(TOOLS)/bin/icetime
ICEPROG = $(TOOLS)/bin/iceprog
ICEBRAM = $(TOOLS)/bin/icebram
SENDBIT = ../../python/send_c3sock.py
VERILATOR = verilator
TECH_LIB = $(TOOLS)/share/yosys/ice40/cells_sim.v

all: $(PROJ).bin

$(FAKE_HEX):
	$(ICEBRAM) -g 32 2048 > $(FAKE_HEX)

%.json: $(SRC) $(FAKE_HEX)
	$(YOSYS) -p 'synth_ice40 -top $(PROJ) -json $@' $(SRC)

%.asc: %.json $(PIN_DEF) 
	$(NEXTPNR) $(NEXTPNR_ARGS) --json $< --pcf $(PIN_DEF) --asc $@
		
$(REAL_HEX):
	$(MAKE) -C ../riscv/c/ main.hex
	cp ../riscv/c/main.hex ./$(REAL_HEX)
		
%.bin: %.asc $(REAL_HEX)
	$(ICEBRAM) $(FAKE_HEX) $(REAL_HEX) < $< > temp.asc
	$(ICEPACK) temp.asc $@

prog: $(PROJ).bin
	$(SENDBIT) $(PROJ).bin

recode:
	rm -f $(REAL_HEX) $(PROJ).bin
	$(MAKE) prog
    
%.rpt: %.asc
	$(ICETIME) -d $(DEVICE) -mtr $@ $<
	
lint: $(SRC)
	$(VERILATOR) --lint-only -Wall --top-module $(PROJ) $(TECH_LIB) $(SRC)

clean:
	rm -f *.json *.asc *.rpt *.bin *.hex

.SECONDARY:
.PHONY: all prog clean
