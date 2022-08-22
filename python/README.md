# Python Scripts For Firmware V0.2
This directory contains the host-side Python utility scripts which are used to
communicate with the ICE-V Wireless board over USB and WiFi.

## Installation
The scripts require Python 3 and use modules that should be available with most
common installations so they should run "out of the box" with no special effort.

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
  -r, --read=REG          : register to read
  -w, --write=REG DATA    : register to write and data to write
      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout
      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>
  -s, --ssid <SSID>       : set WiFi SSID
  -o, --password <pwd>    : set WiFi Password
```

### Fast FPGA programming
To quickly load a new configuration into the FPGA

```
send_c3usb.py <bitstream>
```

### Update default power-on configuration
To load a new default configuration for loading at power-up

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

### Read PSRAM
Reads from the 8MB PSRAM on the board starting at address ADDR for LEN bytes.
Output is sent to stdout so be sure to redirect to a file or pipe that can
handle the raw binary data.

Note that you must have an FPGA design that supports SPI pass-thru to the PSRAM.
A simple example design is available in the Gateware/spi_pass directory.

```
send_c3usb.py --ps_rd=ADDR LEN > <READ FILE>
```

### Write PSRAM
Writes data from <file> to the 8MB PSRAM on the board starting at address ADDR.
At this time file length is maximum 65536 bytes.

Note that you must have an FPGA design that supports SPI pass-thru to the PSRAM.
A simple example design is available in the Gateware/spi_pass directory.

```
send_c3usb.py --ps_wr=ADDR <file>
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
  -p, --port=portnum      : port of FPGA load (default 3333)
  -r, --read=REG          : register to read
  -w, --write=REG DATA    : register to write and data to write
      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout
      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>
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

### Read PSRAM
Reads from the 8MB PSRAM on the board starting at address ADDR for LEN bytes.
Output is sent to stdout so be sure to redirect to a file or pipe that can
handle the raw binary data. At this time LEN is maximum 65536 bytes.

Note that you must have an FPGA design that supports SPI pass-thru to the PSRAM.
A simple example design is available in the Gateware/spi_pass directory.

```
send_c3sock.py --ps_rd=ADDR LEN > <READ FILE>
```

### Write PSRAM
Writes data from <file> to the 8MB PSRAM on the board starting at address ADDR.
At this time file length is maximum 65536 bytes.

Note that you must have an FPGA design that supports SPI pass-thru to the PSRAM.
A simple example design is available in the Gateware/spi_pass directory.

```
send_c3sock.py --ps_wr=ADDR <file>
```
