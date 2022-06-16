# Python
This directory contains the host-side Python utility script which is used to
communicate with the ESP32C3 FPGA board over WiFi.

## Installation
The script uses python modules that should be available with most common installs
so it should run "out of the box" with no special effort.

## Provisioning
The script assumes a default IP address for the ESP32C3 FPGA board of

```
192.168.0.132
```

This will probably not match the DHCP address that your router assigns to the
board when it attaches to the network so you'll either need to provide the
proper address at every invocation of the script with the `--address=<IP>` option
or modify the script to use your address.

## Usage
```
send_c3sock.py [options] [<file>] | [DATA]
  -h, --help            : this message
  -a, --address=ip_addr : address of ESP32C3 (default 192.168.0.132)
  -b, --battery         : report battery voltage (in millivolts)
  -c, --command=[0-15]  : command (default 15 = load FPGA with <file>)
  -f, --flash           : write <file> to SPIFFS flash
  -p, --port=portnum    : port of FPGA load (default 3333)
  -r, --read=REG        : register to read
  -w, --write=REG DATA  : register to write and data to write
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

