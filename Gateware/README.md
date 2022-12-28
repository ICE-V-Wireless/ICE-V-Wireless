# Gateware
This directory contains gateware designs for the ICE-V Wireless.

## Designs
* [Factory Bitstream](factory_bitstream) - the demo gateware that ships with the board.
* [SPI Pass](spi_pass) - a simple SPI pass-thru design that's used by the firmware
to load the PSRAM at boot time.
* [Blinky Demo](blinky_demo) - a very basic LED blinker for learning.

## Prerequisites
Building Gateware requires installation of the OSS CAD Suite as well as a
RISC-V GCC toolchain. Details of this process are coming soon.

## Building
To build these gateware bitstreams:

```
cd <design directory>/icestorm
make
```

That will create a `.bin` file that can be sent to the ICE-V-Wireless board.

## Quick temporary testing the bitstream
After building the design you can quickly load designs into the ICE-V-Wireless
without overwriting the design in flash memory:

```
../../../python/send_c3usb.py <bitstream>
```

where `<bitstream>` is the .bin file created in the Building step above (the
name varies among the different design directories). This loads into the FPGA
immediately and does not require resetting the board.

## Installing the bitstream
It's also possible to load the new bitstream into the flash memory so that it
starts up automatically at power-up:

```
../../../python/send_c3usb.py -f <bitstream>
```

This change doesn't take effect immediately - you'll want to reset the board
to load the new design.

