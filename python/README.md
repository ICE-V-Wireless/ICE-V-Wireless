# Python Scripts For Firmware V0.3

This directory contains the host-side Python utility scripts which are used to
communicate with the ICE-V Wireless board over USB and WiFi. There are currently
three different scripts available:
* send_c3usb.py - communicates with the board over a local USB connection and
allows setting WiFi credentials as well as flashing, "instant" loading and
miscellaneous housekeeping functions.
* send_c3sock.py - communicates with the board over WiFi for flashing, "instant"
loading and miscellaneous houskeeping. Does not allow setting WiFi credentials.
* icevwprog.py - simplified flashing and "instant" loading but no housekeeping
functions. Attempts to autodetect both USB and WiFi connections so it can be used
as a back-end for other devtools that have limited control over connection details.

## Installation

The scripts require Python 3 and use modules that should be available with most
common installations so they should run "out of the box" with no special effort.

## Running under various OSes

These utility scripts are built with Python for maximum cross-platform useability,
however there are still some notable differences in how they can be used. Here
are some usage notes for different operating systems.

### Linux

Linux is the "native" system under which these scripts were developed and as a
result this will be the smoothest use-case. No special effort should be required since the Python serial library will should automatically find your ICE-V-Wireless module using its USB VID:PID values.

### Mac OS

In general Mac OS behaves very similarly to Linux when running Python scripts. On
older versions it may be necessary to install Python 3, but simple packages are
available directly from Python.org

https://www.python.org/downloads/macos/

Once that has been installed you may also need to install the Pyserial module

`pip3 install pyserial`

Then it will likely be necessary to prefix the commands with `python3` to ensure
the correct version & location is used.

Determining the serial device name for Mac OS can be a bit challenging. The
easiest approach is to list the serial device before and after plugging in the
USB cable and using the one that appears:

`ls /dev/c*`

Then use that as an argument to the USB script. For example:

`python3 ./send_c3usb.py -p /dev/tty.cxxx -i`

### Windows

Windows is the most problematic OS for these scripts, but it can work with some
care.

First, you'll need to install Python 3 from the Windows marketplace. It's
possible to install by downloading from Python.org, but experience shows that
doesn't always result in success due to the way environment variables are set up.
The simple approach on Windows 11 is to open a Command prompt and type:

`python.exe`

If Python is not installed then a window will open offering to install it.
Follow the instructions to complete the install.

Once Python is installed and working from the Windows Command prompt it will be
necessary to install the pyserial module:

`pip3 install pyserial`

At this point python should be properly set up and you should be able to run
the scripts from within the `python` directory of this repo by prepending
`python3` to the scripts.

Determining the serial port to use can be difficult. Use the Device Manager to
find the COM port that's added when the ICE-V is plugged in and use that with
the port option of the USB Serial script:

`python3 send_c3usb.py -p COMx -i`

where `COMx` is the port name you discovered in the Device Manager.

#### Note

There is a quirk in the Windows Pyserial module that causes the ICE-V board to
reset at the completion of every command received from the USB Serial script. It
will be necessary to wait several seconds for the green activity LED to start
flashing again after every command sent via the USB serial script.

When using the WiFi script it may be necessary to explicitly provide the IPV4
address of the device. Using the `-i` command from the USB Serial script you
can determine the IPV4 address and then provide that to the WiFi script:

`python3 send_c3sock.py -a <YOUR_IP_ADDR> -b`

## USB Serial

The script `send_c3usb.py` communicates with the ICE-V-Wireless board via
USB and requires only that you attach the board to a host computer and wait
until the green `C3` LED is flashing. 

### USB Usage

```
send_c3usb.py [options] [<file>] | [DATA]
  -h, --help              : this message
  -p, --port=<tty>        : usb tty of ESP32C3 (default /dev/ttyACM0)
  -b, --battery           : report battery voltage (in millivolts)
  -f, --flash=<file>      : write <file> to SPIFFS flash
  -i, --info              : get info (version, IP addr)
  -l, --load=<cfg#>       : load config from SPIFFS (0=default, 1=spi_pass)
  -r, --read=REG          : register to read
  -w, --write=REG DATA    : register to write and data to write
      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout
      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>
      --ps_in=ADDR <file> : write PSRAM init at ADDR with data in <file>
  -s, --ssid <SSID>       : set WiFi SSID
  -o, --password <pwd>    : set WiFi Password
```

### Fast FPGA programming

To quickly load a new configuration into the FPGA

```
send_c3usb.py <bitstream>
```

### Update default power-on configuration

To load a new default configuration into SPIFFS for loading at power-up

```
send_c3usb.py --flash <bitstream>
```

### Read battery voltage

To get the current LiPo batter voltage value in millivolts

```
send_c3usb.py --battery
```

### Read Info

To get the firmware version and WiFi IP address

```
send_c3usb.py --info
```

### Read a SPI register

If the current FPGA design supports SPI CSRs, read a register

```
send_c3usb.py --read=<REG>
```

### Write a SPI register

If the current FPGA design supports SPI CSRs, write a register

```
send_c3usb.py --write=<REG> <DATA>
```

### Load a stored configuration

This configures the FPGA with a configuration already stored in the onboard SPIFFS filesystem. Choose <cfg#> 0 for the default configuration and 1 for the "spi_pass" configuration which is needed prior to issuing the PSRAM commands below.

```
send_c3usb.py --load=<cfg#>
```

### Read PSRAM

Reads from the 8MB PSRAM on the board starting at address ADDR for LEN bytes.
Output is sent to stdout so be sure to redirect to a file or pipe that can
handle the raw binary data.

Note that you must have an FPGA design that supports SPI pass-thru to the PSRAM - the easiest way to do this is by issuing the `--load=1` prior.

```
send_c3usb.py --ps_rd=ADDR LEN > <READ FILE>
```

### Write PSRAM

Writes data from <file> to the 8MB PSRAM on the board starting at address ADDR.
At this time file length is limited to the free SRAM in the ESP32C3 which is about 200kB.

Note that you must have loaded an FPGA design that supports SPI pass-thru to the PSRAM - the easiest way to do this is by issuing the `--load=1` command prior.

```
send_c3usb.py --ps_wr=ADDR <file>
```

### Write PSRAM Init

Writes data from <file> to the SPIFFS filesystem for loading at powerup into the 8MB PSRAM on the board starting at address ADDR.
At this time the file length is limited to roughly the free space in the SPIFFS filesystem which is about 560kB.

```
send_c3usb.py --ps_in=ADDR <file>
```

### Set WiFi SSID

Sets the WiFi SSID credential to use when first connecting at power-up.

```
send_c3usb.py --ssid <YOUR SSID>
```

Note that after setting the SSID and Password you will need to reset the
ICE-V-Wireless board to allow it to connect with the new credentials. Once
the board has successfully connected the green `C3` LED will flash at a
5Hz rate to show that WiFi is working. If WiFi is not connected then only
USB Serial connections will be possible and the LED will blink at a 1Hz rate.

### Set WiFi Password

Sets the WiFi Password credential to use when first connecting at power-up.

```
send_c3usb.py --password <YOUR PASSWORD>
```

Note that after setting the SSID and Password you will need to reset the
ICE-V-Wireless board to allow it to connect with the new credentials. Once
the board has successfully connected the green `C3` LED will flash at a
5Hz rate to show that WiFi is working. If WiFi is not connected then only
USB Serial connections will be possible and the LED will blink at a 1Hz rate.

## WiFi

The script `send_c3sock.py` communicates with the ICE-V-Wireless board via
WiFi networking and requires that you have previously provided your WiFi network
credentials before it can join your network. The ICE-V default firmware uses mDNS
to advertise its IP address and a special "_FPGA" service on port 3333 so the
script assumes a default IP address for the ICE-V Wireless board of:

```
ICE-V.local
```

If your system doesn't support mDNS then you'll need to query your DHCP server to
find the IP address that it assigned to the ICE-V and subsequently provide the
proper address at every invocation of the script with the `--address=<IP>` option
or modify the script to use that address.

### WiFi Usage

```
send_c3sock.py [options] [<file>] | [DATA]
  -h, --help              : this message
  -a, --address=ip_addr   : address of ESP32C3 (default ICE-V.local)
  -b, --battery           : report battery voltage (in millivolts)
  -f, --flash <file>      : write <file> to SPIFFS flash
  -i, --info              : get info (version, IP addr)
  -l, --load=<cfg#>       : load config from SPIFFS (0=default, 1=spi_pass)
  -p, --port=portnum      : port of FPGA load (default 3333)
  -r, --read=REG          : register to read
  -w, --write=REG DATA    : register to write and data to write
      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout
      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>
      --ps_in=ADDR <file> : write PSRAM init at ADDR with data in <file>
```

### Fast FPGA programming

To quickly load a new configuration into the FPGA

```
send_c3sock.py <bitstream>
```

### Update default power-on configuration

To load a new default configuration for loading at power-up

```
send_c3sock.py --flash <bitstream>
```

### Read battery voltage

To get the current LiPo batter voltage value in millivolts

```
send_c3sock.py --battery
```

### Read Info

To get the firmware version and WiFi IP address

```
send_c3sock.py --info
```

### Read a SPI register

If the current FPGA design supports SPI CSRs, read a register

```
send_c3sock.py --read=<REG>
```

### Write a SPI register

If the current FPGA design supports SPI CSRs, write a register

```
send_c3sock.py --write=<REG> <DATA>
```

### Load a stored configuration

This configures the FPGA with a configuration already stored in the onboard SPIFFS filesystem. Choose <cfg#> 0 for the default configuration and 1 for the "spi_pass" configuration which is needed prior to issuing the PSRAM commands below.

```
send_c3sock.py --load=<cfg#>
```

### Read PSRAM

Reads from the 8MB PSRAM on the board starting at address ADDR for LEN bytes.
Output is sent to stdout so be sure to redirect to a file or pipe that can
handle the raw binary data.

Note that you must have loaded an FPGA design that supports SPI pass-thru to the PSRAM - the easiest way to do this is by issuing the `--load=1` command prior.

```
send_c3sock.py --ps_rd=ADDR LEN > <READ FILE>
```

### Write PSRAM

Writes data from <file> to the 8MB PSRAM on the board starting at address ADDR.
At this time file length is maximum 65536 bytes.

Note that you must have loaded an FPGA design that supports SPI pass-thru to the PSRAM - the easiest way to do this is by issuing the `--load=1` command prior.

```
send_c3sock.py --ps_wr=ADDR <file>
```

### Write PSRAM Init

Writes data from to the SPIFFS filesystem for loading at powerup into the 8MB PSRAM on the board starting at address ADDR.
At this time the file length is limited to roughly the free space in the SPIFFS filesystem which is about 560kB.

```
send_c3sock.py --ps_in=ADDR <file>
```

## icevwprog.py
A simplified interface for loading and flashing which attempts to autodetect
the interface (either USB or WiFi). This may be useful as a back-end for some
FPGA development tools that have limited flexibility when setting up the
connection to the target board.
```
./icevwprog.py  [options] <file>
  -h, --help              : this message
  -d, --device=<DEV>      : device ID to connect to
  -S, --sram              : program to FPGA SRAM instead of ESP32 Flash
  -v, --verbose           : show work
```

The <DEV> device ID is a flexible identifier that can select various connections
but is not usually required. The details are as follows:
* For autodetection (the default) use <DEV> = auto
* To force a USB connection use <DEV> = usb
  * for a specific USB device port use <DEV> = usb:port (eg usb:/dev/ttyACM0)
* To force a WiFi connection use <DEV> = wifi
  * for a specific WiFi device address use <DEV> = wifi:IP_ADDR (eg wifi:192.168.0.100)

### Fast FPGA programming

To quickly load a new configuration into the FPGA

```
icevwprog.py -S <bitstream>
```

### Update default power-on configuration

To flash a new default configuration for loading at power-up

```
icevwprog.py <bitstream>
```

