# Gateware
This directory contains source and build materials necessary to create the FPGA
bitstream for a demonstration gateware compatible with the ESP32C3 FPGA design.

## Prerequisites
You will need the Open Source FPGA toolchain to build this. You can download it
here:

https://github.com/YosysHQ/oss-cad-suite-build

## Building
Before building you must modify the Makefile to point to your installation of the
OSS CAD Suite. Edit the file 'icestorm/Makefile' and modify the line

TOOLS = /opt/openfpga/fpga-toolchain

to point to the location where you have installed the tools

Once that is complete you may cd to the icestorm directory and run make. Note that
make process may abort with an error due to missed timing. If that occurs
simply rerun make to complete the binary creation.

## Installing

The result of the 'make' process above should be a binary entitled 'up5k_esp32c3.bin'
which may be installed using 'make prog'. Note that this process depends on
the python host utility being in the Python directory of this repo and that it has
been modified to use the proper local network address for your board.

