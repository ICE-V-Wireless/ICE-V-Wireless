# Firmware Version 0.2
This directory contains the ESP32 C3 firmware for the ICE-V-Wireless board. The
default firmware supports power-on initialization of the ICE40UP5k FPGA, as well
as USB Serial and WiFi programming of the FPGA, SPI access after configuration,
PSRAM access and battery voltage monitoring. Additionally, the WiFi credentials
can be set via USB Serial.

## Prerequisites

### ESP32 Tools
Prior to building this firmware you'll need to have a working installation of the
Espressif ESP32 IDF toolchain. Unfortunately there are many different versions
available and not all components and features used here are compatible across
them all. This project was built against the V4.4.2 stable release.
You can find it here:

https://github.com/espressif/esp-idf

Make sure you get `stable (v4.4.2)` and that you've done all the proper
installation and setup steps.

NOTE: The github CI process for this repo uses a Docker image for esp-idf that's
provided by Espressif and appears to contain a late version of V4.4.1 that
does work but requires some preprocessor fiddling to get full function.

## Building
### First build
For the initial build it is necessary to install the SPIFFS filesystem that holds
the default FPGA build. To do this it is necessary to uncomment a line in the
`main/CMakeLists.txt` file which informs the build system to create and flash
the filesystem into the ESP32C3 memory. Find the line

```
#spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
```
and remove the leading `#`

### Subsequent builds
After the first build the SPIFFS filesystem will exist and the firmware will know
where to find it so it's no longer necessary to recreate it every time the
firmware is updated. Return to the `main/CMakeLists.txt` and add a `#` to the
beginning of the above text line.

### How to build
Use the normal IDF build command:
```
idf.py build
```

## Installation
Connect the USB-C port of the ESP32C3 FPGA board to the build host machine and
use the command
```
idf.py -p <serial device> flash
````

where `<serial device>` is the USB serial device which is created when the board
enumerates.

## Monitoring
The board generates a fair amount of status information that is useful for
debugging and tracking performance. Use the command
```
idf.py -p <serial device> monitor
```
to view this information.

### NOTE
Enabling the monitor function interferes with the USB Serial command/configuration
utility. If you intend to use USB Serial to communicate with the ICE-V-Wireless
board then *DO NOT USE* the IDF monitoring utility at the same time.

## USB Operation
Connect the ICE-V-Wireless board to a host computer via the USB cable to provide
power and wired communication. A Python script `send_c3usb.py` is provided which
presents a user-friendly command line interface and is described in more detail
here:
[python utility](../python/README.md)

## WiFi Provisioning
At compile time the firmware has "safe" default WiFi credentials that will
likely not work with any real network:
* SSID = "MY_SSID"
* Password = "MY_PASSWORD"

In order to set values for your network you should use the USB Serial
command-line Python script for communicating with the board. The script
can be found in the `python` directory of this repository. The syntax for
setting values is:

`send_c3usb.py --ssid <YOUR SSID>`
`send_c3usb.py --password <YOUR PASSWORD>`

After successfully executing these commands without errors you should then push
the `RST` button on the ICE-V-Wireless board to let it join your network. When
the LED is flashing at a quick ~5Hz rate you will know that the board was able
to successfully join the network. A slow ~1Hz rate indicates that only the
USB Serial interface is active.

## IP Addressing
This firmware uses DHCP and mDNS to request an IP address from the router on the
WiFi network and to advertise the address and its specific service/socket. The
ICE-V-Wireless board can be found at `ICE-V.local`. If your system doesn't
support mDNS then you'll need to query your DHCP server to find the IP address
that it assigned to the client with the hostname 'espressif'.

## WiFi Operation
Once the firmware is running and connected to a WiFi network it creates a TCP
socket on port 3333 which can be accessed without any security measures by any
peer on the network. The socket accepts commands of the form

```
[Header][Size][Data]
```
* Header is a 32-bit value of 0xCAFEBEEX where X is any 4-bit value from 0-15
that defines how the following data will be interpreted.
  * 0 = SPI Read Register where Data consists of one byte containing the 7-bit
register address to read. An error status byte followed by four bytes containing
the 32-bit register contents is returned on the same socket.
  * 1 = SPI Write Register where Data consists of one byte containing the 7-bit
register address to write followed by four bytes containing the 32-bit value to
write. An error status byte is returned on the same socket.
  * 2 = Vbat report which requires no Data. An error status byte, followed by two
bytes containing the battery voltage in millivolts is returned on the same socket.
  * 11 = Read PSRAM where Data consists of four bytes defining the starting
address in PSRAM at which to begin reading, followed by the number of bytes to
read. An error status byte, followed by the requested read data is returned on
the same socket.
  * 12 =  Write PSRAM where Data consists of four bytes defining the starting
address in PSRAM at which to write followed by the actual write data. An error
status byte is returned on the same socket
  * 14 = Write FPGA default power-on bitstream to SPIFFS where Data consists of
the 104090-byte FPGA configuration bitstream. An error status byte is returned
on the same socket.
  * 15 = Write FPGA live bitstream directly to the FPGA where Data consists of
the 104090-byte FPGA configuration bitstream. An error status byte is returned
on the same socket.
  * All other command values are reserved for future use.
* Size is a 32-bit value definining the number of bytes following, excluding the
header and size values.
* Data is a stream of bytes of length Size.

## Usage
A Python script `send_c3sock.py` is provided which presents a user-friendly
command line interface to the TCP socket and is described in more detail here:
[python utility](../python/README.md)
