# Pinout
This directory contains source materials necessary to create a handy pinout
diagram for the ICE-V Wireless (a derivative of the ESP32C3-FPGA design). To
build this you will need to have installed the Python 'pinout' utility. Get
that here:

https://github.com/j0ono0/pinout

## Building
To create the pinout diagram, execute the following command:
```
py -m pinout.manager --export pinout_diagram.py diagram.png
```
This will create a file `diagram.png` in the current directory
