# Gateware
This directory contains source and build materials necessary to create the FPGA
bitstream for a demonstration gateware compatible with the ICE-V Wireless FPGA
design.

## Design Details
This demonstration design consists of several parts:
* A SPI peripheral interface that can be controlled by the ESP32C3 module which
provides up to 128 32-bit CSRs. In this design there are seven addresses used
with two R/W registers and five read-only register.
* A RISC-V soft-core MCU with the following features:
  * 64kB SRAM
  * 8kB ROM
  * Up to 128 bits GPIO
  * 115200bps UART
  * 2 SPI ports
  * 2 I2C ports
  * Clock cycle counter
* 8-bit wide FIFO mailbox for communication between the soft-core and ESP32C3
* RGB LED with PWM brighness control driven by the soft-core

Firmware for the RISC-V soft core is coded in pure C and does simple UART
and RGB test I/O.

## Prerequisites
You will need the Open Source FPGA toolchain to build this. You can download it
here:

https://github.com/YosysHQ/oss-cad-suite-build

The RISC-V firmware build requires a `riscv-64-unknown-elf` toolchain. You can
use the one that's available from the SiFive github:

https://github.com/sifive/freedom-tools/releases

## Building
Before building you must modify the Makefile to point to your installation of the
OSS CAD Suite. Edit the file 'icestorm/Makefile' and modify the line

TOOLS = /opt/openfpga/fpga-toolchain

to point to the location where you have installed the tools

Once that is complete you may change to the icestorm directory and run

`make`

## Installing

The result of the 'make' process above should be a binary entitled 'bitstream.bin'
which may be installed using 'make prog'. Note that this process depends on
the python host utility being in the Python directory of this repo and that any
required modifications have been made so that it uses the proper local network
address for your board.

