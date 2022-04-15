# ESP32C3_FPGA
Combined ESP32C3 and iCE40 FPGA board

<img src="docs/3D_front.png" width="640" />
<img src="docs/3D_back.png" width="640" />

## Abstract
This project combines an Espressif ESP32C3HN4 SoC (which includes 4MB of
flash in the package) with a Lattice iCE40 FPGA (either an iCE5LP4k or
iCE40UP5k or other pin an package-compatible devices) to allow WiFi and
Bluetooth control of the FPGA configuration and control. ESP32 and FPGA
I/O is mostly uncommitted except for the pins used for SPI communication.
Several of the ESP32C3 GPIO pins are available for additonal interfaces
such as ADC, I2C, etc.

This project comprises both hardware, firmware, gateware and host-side
communication utilities.

## Hardware
The hardware design is provided as schematic and layout in KiCAD 6 format.

## Firmware
The ESP32 firmware is written in C with the ESP-IDF V5.0 toolchain and
libraries. Initially it provides a TCP socket interface over WiFi with the
following features:
* Initial loading of the FPGA configuration at powerup from a SPIFFS filesystem
contained in the ESP32C3HN4 flash.
* WiFi socket-based "instant" loading of configurations direct to the FPGA.
* WiFi socket-based updating of the configuration stored in SPIFFS.
* WiFi socket-based monitor/control of the FPGA design via SPI.

## Gateware
The iCE40 FPGA gateware provided is a simple design that demonstrates basic
SPI monitor/control via SPI with a flashing LED. More advanced designs are
possible.

## Host-side Utilities
A command-line Python script is provided to communicate over TCP sockets
with the ESP32C3 which supports the features outlined in the Firmware section
above.

