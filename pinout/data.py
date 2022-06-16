legend = [
    ("Analog", "analog"),
    ("Ground", "gnd"),
    ("GPIO", "gpio"),
    ("NRST", "nrst"),
    ("Power", "pwr"),
    ("FPGA", "fpga"),
    ("FPGAP", "fpgap"),
    ("FPGAN", "fpgan"),
]

# Pinlabels

j3 = [
    [
        ("4V", "pwr"),
    ],
    [
        ("3.3V", "pwr"),
    ],
    [
        ("NRST", "nrst"),
    ],
    [
        ("2", "analog"),
    ],
    [
        ("3", "analog"),
    ],
    [
        ("8", "gpio"),
    ],
    [
        ("9", "gpio"),
    ],
    [
        ("10", "gpio"),
    ],
    [
        ("20", "gpio"),
    ],
    [
        ("21", "gpio"),
    ],
    [
        ("P34", "fpga"),
    ],
    [
        ("GND", "gnd"),
    ],
]

pmod1 = [
    [
        ("P18", "fpgan"),
        ("P19", "fpgap"),
    ],
    [
        ("P21", "fpgan"),
        ("P12", "fpgap"),
    ],
    [
        ("P10", "fpga"),
        ("P11", "fpga"),
    ],
    [
        ("P6", "fpga"),
        ("P9", "fpga"),
    ],
    [
        ("GND", "gnd"),
        ("GND", "gnd"),
    ],
    [
        ("VPWR", "pwr"),
        ("VPWR", "pwr"),
    ],
]

pmod2 = [
    [
        ("P3", "fpgan"),
        ("P4", "fpgap"),
    ],
    [
        ("P45", "fpgan"),
        ("P48", "fpgap"),
    ],
    [
        ("P44/GCLK6", "fpgan"),
        ("P47", "fpgap"),
    ],
    [
        ("P2", "fpga"),
        ("P46", "fpga"),
    ],
    [
        ("GND", "gnd"),
        ("GND", "gnd"),
    ],
    [
        ("VPWR", "pwr"),
        ("VPWR", "pwr"),
    ],
]

pmod3 = [
    [
        ("P38", "fpga"),
        ("P42", "fpga"),
    ],
    [
        ("P36", "fpgan"),
        ("P43", "fpgap"),
    ],
    [
        ("P31", "fpgan"),
        ("P32", "fpgap"),
    ],
    [
        ("P20/GCLK3", "fpgan"),
        ("P13", "fpgap"),
    ],
    [
        ("GND", "gnd"),
        ("GND", "gnd"),
    ],
    [
        ("VPWR", "pwr"),
        ("VPWR", "pwr"),
    ],
]

# Text

title = "<tspan class='h1'>ICE-V Wireless</tspan>"

description = """ESP32C3 and iCE40UP5K FPGA for WiFi and 
Bluetooth FPGA interfacing. More info at 
<tspan class='italic strong'>https://github.com/mwelling/ICE-V-Wireless</tspan>"""
