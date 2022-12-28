# Makefile for Icarus Verilog simulation
# 03-19-2021 E. Brombaugh

# sources
SOURCES = 	tb_system.v ../src/system.v ../src/acia.v ../src/acia_tx.v \
			../src/acia_rx.v ../src/bram_512x32.v ../src/spram_16kx32.v \
			../picorv32/picorv32.v ../src/wb_bus.v ../src/wb_master.v \
			../src/mailbox.v ../src/fifo1.v
#SOURCES = 	tb_system.v ../icestorm/system_struct.v

# top level
TOP = tb_system
ROM = rom.hex
			
# Executables
TOOLS = /opt/openfpga/fpga-toolchain
VLOG = iverilog
WAVE = gtkwave
TECH_LIB = $(TOOLS)/share/yosys/ice40/cells_sim.v

# targets
all: $(TOP).vcd

wave: $(TOP).vcd $(TOP).gtkw
	$(WAVE) $(TOP).gtkw

$(ROM):
	$(MAKE) -C ../c/ main.hex
	cp ../c/main.hex ./$(ROM)

$(TOP).vcd: $(TOP) $(ROM)
	./$(TOP)

$(TOP): $(SOURCES)
	$(VLOG) -D icarus -DNO_ICE40_DEFAULT_ASSIGNMENTS -l $(TECH_LIB) -o $(TOP) $(SOURCES)
	
clean:
	rm -rf a.out *.obj $(ROM) $(TOP) $(TOP).vcd
	
