# Makefile for icestorm tools + yosys + nextpnr
# Modified from examples in icestorm distribution
# 01-16-18 E. Brombaugh

# src directory
VPATH = ../src

SRC =	../src/bitstream.v ../src/spi_slave.v \
		../src/system.v ../src/picorv32.v \
		../src/spram_16kx32.v \
		../src/acia.v ../src/acia_tx.v ../src/acia_rx.v \
		../src/wb_bus.v ../src/wb_master.v \
		../src/mailbox.v ../src/fifo1.v

# preparing the machine code
FAKE_HEX =	rom.hex
REAL_HEX =  code.hex

# project stuff
PROJ = bitstream
PIN_DEF = ../src/bitstream.pcf
SDC = ../src/bitstream.sdc
DEVICE = up5k
PACKAGE = sg48

TOOLS ?= /opt/openfpga/fpga-toolchain
YOSYS = $(TOOLS)/bin/yosys
NEXTPNR = $(TOOLS)/bin/nextpnr-ice40
NEXTPNR_ARGS = --pre-pack $(SDC) --package $(PACKAGE) --$(DEVICE) --timing-allow-fail
ICEPACK = $(TOOLS)/bin/icepack
ICETIME = $(TOOLS)/bin/icetime
ICEPROG = $(TOOLS)/bin/iceprog
ICEBRAM = $(TOOLS)/bin/icebram
SENDBIT = ../../../python/send_c3sock.py
VERILATOR = verilator
TECH_LIB = $(TOOLS)/share/yosys/ice40/cells_sim.v

all: $(PROJ).bin

$(FAKE_HEX): $(REAL_HEX)
#	$(ICEBRAM) -g 32 2048 > $(FAKE_HEX)
	cp $(REAL_HEX) $(FAKE_HEX)
	
%.json: $(SRC) $(FAKE_HEX)
	$(YOSYS) -p 'synth_ice40 -top $(PROJ) -json $@' $(SRC)

%.asc: %.json $(PIN_DEF) 
	$(NEXTPNR) $(NEXTPNR_ARGS) --json $< --pcf $(PIN_DEF) --asc $@
		
$(REAL_HEX):
	$(MAKE) -C ../c/ main.hex
	cp ../c/main.hex ./$(REAL_HEX)
		
%.bin: %.asc $(REAL_HEX)
#	$(ICEBRAM) $(FAKE_HEX) $(REAL_HEX) < $< > temp.asc
#	$(ICEPACK) temp.asc $@
	$(ICEPACK) $< $@
	
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
	$(MAKE) -C ../c/ clean
	rm -f *.json *.asc *.rpt *.bin *.hex

.SECONDARY:
.PHONY: all prog clean
