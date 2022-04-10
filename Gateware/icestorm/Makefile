# Makefile for icestorm tools + yosys + nextpnr
# Modified from examples in icestorm distribution
# 02-28-19 E. Brombaugh

# src directory
VPATH = ../src

SRC = ../src/lp4k_bob.v ../src/spi_slave.v

PROJ = lp4k_bob
PIN_DEF = ../src/lp4k_bob.pcf
DEVICE = u4k

YOSYS = yosys
NEXTPNR = nextpnr-ice40
#NEXTPNR_ARGS = --pre-pack $(SDC) --ignore-loops
#NEXTPNR_ARGS =  --pre-pack $(SDC)
NEXTPNR_ARGS =  --pcf-allow-unconstrained
ICEPACK = icepack
ICETIME = icetime
ICEPROG = iceprog
CDCPROG = cdc_prog
VERILATOR = verilator
TECH_LIB = /usr/local/share/yosys/ice40/cells_sim.v

all: $(PROJ).bin

%.json: $(SRC)
	$(YOSYS) -p 'synth_ice40 -top $(PROJ) -json $@' $(SRC)

%.asc: %.json $(PIN_DEF) 
	$(NEXTPNR) $(NEXTPNR_ARGS) --$(DEVICE) --json $< --pcf $(PIN_DEF) --asc $@

%.bin: %.asc
	$(ICEPACK) $< $@

%.rpt: %.asc
	$(ICETIME) -d $(DEVICE) -mtr $@ $<

#prog: $(PROJ).bin
#	$(ICEPROG) -S $<
    
prog: $(PROJ).bin
	$(CDCPROG) -p /dev/ttyACM0 $<

sudo-prog: $(PROJ).bin
	@echo 'Executing prog as root!!!'
	sudo $(ICEPROG) -S -IB $<

clean:
	rm -f $(PROJ).json $(PROJ).asc $(PROJ).rpt $(PROJ).bin

.SECONDARY:
.PHONY: all prog clean
