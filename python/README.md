# Python
This directory contains the host-side Python utility script which is used to
communicate with the ICE-V Wireless board over WiFi.

## Installation
The script uses python modules that should be available with most common installs
so it should run "out of the box" with no special effort.

## Addressing
The ICE-V default firmware uses mDNS to advertise its IP address and a special
"_FPGA" service on port 3333 so the script assumes a default IP address for the
ICE-V Wireless board of:

```
ICE-V.local
```

If your system doesn't support mDNS then you'll need to query your DHCP server to
find the IP address that it assigned to the ICE-V and subsequently provide the
proper address at every invocation of the script with the `--address=<IP>` option
or modify the script to use that address.

## Usage
```
send_c3sock.py [options] [<file>] | [DATA]
  -h, --help              : this message
  -a, --address=ip_addr   : address of ESP32C3 (default ICE-V.local)
  -b, --battery           : report battery voltage (in millivolts)
  -f, --flash <file>      : write <file> to SPIFFS flash
  -p, --port=portnum      : port of FPGA load (default 3333)
  -r, --read=REG          : register to read
  -w, --write=REG DATA    : register to write and data to write
      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout")
      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>")
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
