# Blinky Demo
This is a demonstration gateware that provides a minimal example of writing to the PSRAM.

## Design overview
The design relies on the built-in 48MHz high-frequency oscillator of the ice40up5k
FPGA to clock a custom SPI master that writes fixed data to the PSRAM. Though there are hardware SPI controllers, used in [Factory Bitstream](factory_bitstream), these are unnecessarily complicated for this basic example.

Note that the PSRAM is only accessible from the FPGA, so in order to read the written data from your PC, you will need to load the [SPI Pass](spi_pass) design, after which `send_c3usb.py --ps_rd=ADDR LEN` can be used.

## Notable Details
The 10kHz oscillator and RGB driver are instantiated library elements which are
described in detail in the Lattice ice40 documentation. The module definitions
are found in the OSS CAD Suite Yosys shared directory
```
oss-cad-suite/share/yosys/ice40/cells_sim.v
```
