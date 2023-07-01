# Makefile for icestorm tools + yosys + nextpnr
# Modified from examples in icestorm distribution
# 01-16-18 E. Brombaugh
# 07-01-23 modified to use VHDL

SRC =	../src/psram_demo.vhd
SRC2 =	../src/spi_master.vhd

# project stuff
PROJ = psram_demo
PIN_DEF = ../src/psram_demo.pcf
SDC = ../src/psram_demo.sdc
DEVICE = up5k
PACKAGE = sg48

TOOLS = /opt/openfpga/fpga-toolchain
YOSYS = $(TOOLS)/bin/yosys
NEXTPNR = $(TOOLS)/bin/nextpnr-ice40
NEXTPNR_ARGS = --pre-pack $(SDC) --package $(PACKAGE) --$(DEVICE) --timing-allow-fail
ICEPACK = $(TOOLS)/bin/icepack
ICETIME = $(TOOLS)/bin/icetime
ICEPROG = $(TOOLS)/bin/iceprog
ICEBRAM = $(TOOLS)/bin/icebram
SENDBIT = ../../../python/send_c3usb.py
VERILATOR = verilator
TECH_LIB = $(TOOLS)/share/yosys/ice40/cells_sim.v
MODULE = -m ghdl

all: $(PROJ).bin

analyse:
	ghdl -a --std=93c --ieee=synopsys -fexplicit -g $(SRC)	
	ghdl -a --std=93c --ieee=synopsys -fexplicit -g $(SRC2)	

%.json: analyse 
	$(YOSYS) $(MODULE) -p 'ghdl psram_demo; synth_ice40 -top $(PROJ) -json $@'

%.asc: %.json $(PIN_DEF) 
	$(NEXTPNR) $(NEXTPNR_ARGS) --json $< --pcf $(PIN_DEF) --asc $@

%.bin: %.asc
	$(ICEPACK) $< $@

prog: $(PROJ).bin
	python3.11 $(SENDBIT) $(PROJ).bin

flash: $(PROJ).bin
	$(SENDBIT) -f $(PROJ).bin

%.rpt: %.asc
	$(ICETIME) -d $(DEVICE) -mtr $@ $<
	
lint: $(SRC)
	$(VERILATOR) --lint-only -Wall --top-module $(PROJ) $(TECH_LIB) $(SRC)

clean:
	rm -f *.json *.asc *.rpt *.bin *.hex

.SECONDARY:
.PHONY: all prog clean
