# Blinky Demo
This is a simple demonstration gateware that provides a minimal example of
blinking the RGB LED.

## Design overview
The design relies on the built-in 10kHz low-frequency oscillator of the ice40up5k
FPGA to clock a prescaler that generates a 1Hz timing pulse. That timing pulse
drives a 3-bit counter which controls the RGB LED outputs. The result is that
the LED will cycle through 8 colors (black,red,yellow,green,blue,magenta,cyan,white)
over the course of 8 seconds.

## Notable Details
The 10kHz oscillator and RGB driver are instantiated library elements which are
described in detail in the Lattice ice40 documentation. The module definitions
are found in the OSS CAD Suite Yosys shared directory
```
oss-cad-suite/share/yosys/ice40/cells_sim.v
```
