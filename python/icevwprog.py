#!/usr/bin/env python3
# program ICE-V-Wireless via USB or socket
# 10-2-22 E. Brombaugh

import sys
import os
import getopt
import socket
import serial
from serial.tools import list_ports
import ping3

# get ESP32C3 serial port
def get_C3_port():
    port = list(list_ports.grep("303a:1001"))
    if len(port) > 0:
        return port[0][0]
    else:
        return 'none'

# determine serial port from device string
def get_serial_port_from_device(dev):
    colon = dev.find(":")
    port = "auto"
    if colon != -1:
        port = dev[colon+1:]
    if port.lower() == "auto":
        port = get_C3_port()
    return port

# determine IP from device string
def get_IP_from_device(dev):
    colon = dev.find(":")
    ip = "auto"
    if colon != -1:
        ip = dev[colon+1:]
    if ip.lower() == "auto":
        ip = "ice-v.local"
    return ip

# convert a command nybble into a 32-bit magic value for the header
def make_magic(cmmd):
    cmmd = cmmd & 15
    return bytearray([0xE0+cmmd, 0xBE, 0xFE, 0xCA])

# receive error and tokens
def recv_err_tokens(ptype, port):
    reply = tty.read_until()

    # reply has to have at least 6 chars to be valid but sometimes
    # has garbage from ESP32 logging at beginning
    if len(reply) >= 6:
        # convert binary to ascii string and tokenize
        rplystr = reply.decode('utf-8')
        rplytok = rplystr.split()
        
        # search tokens for reply header
        for tokidx in range(len(rplytok)):
            if rplytok[tokidx] == 'RX':
                # found header so return happy
                err = int(rplytok[tokidx+1], 16)
                return err, rplytok[tokidx+2:]
        else:
            # didn't find header
            return 64, 0
    else:
        # too short
        return 32, 0
    
# send a file for direct load to FPGA or write to SPIFFS
def send_file(name, cmmd, ptype, port):
    # open file as binary
    with open(name, "rb") as file:
        # get length by seeking around
        file.seek(0, os.SEEK_END)
        file_len = file.tell()
        file.seek(0, os.SEEK_SET)

        if verbose:
            print("send_file() - Command", cmmd, "Size of", name, "is", file_len, "bytes")

        # add the header with command
        magic = make_magic(cmmd)
        size = file_len.to_bytes(4, byteorder = 'little')
        payload = b"".join([magic, size, file.read(file_len)])

        # send payload to the C3 by chosen transport and get response
        if ptype == "serial":
            # serial uses tty already opened
            size = len(payload)
            while size:
                written = tty.write(payload)
                size = size - written
                payload = payload[written:]
            err, toks = recv_err_tokens(ptype, port)
        else:
            # send to the socket server on the C3
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((port, 3333))
                s.sendall(payload)
                reply = s.recv(1024)
                err = reply[0]
                s.close()

        if err:
            print("Error", err)

# send a load command plus config ID
def load_cfg(reg, ptype, port):
    if verbose:
        print("load_cfg(", reg, ")")

    magic = make_magic(6)
    regsz = 4
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send payload to the C3 by chosen transport and get response
    if ptype == "serial":
        # serial uses tty already opened
        size = len(payload)
        while size:
            written = tty.write(payload)
            size = size - written
            payload = payload[written:]
        err, toks = recv_err_tokens(ptype, port)
    else:
        # send to the socket server on the C3
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((port, 3333))
            s.sendall(payload)
            reply = s.recv(1024)
            err = reply[0]
            s.close()

    if err:
        print("Error", err)

# usage text for command line
def usage():
    print(sys.argv[0], " [options] <file>")
    print("  -h, --help              : this message")
    print("  -d, --device=<DEV>      : device ID to connect to")
    print("  -S, --sram              : program to FPGA SRAM instead of ESP32 Flash")
    print("  -v, --verbose           : show work")

# main entry
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], \
            "hd:Sv", \
            ["help", "device=", "sram", "verbose"])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # defaults
    dev = "auto"
    verbose = False
    cmmd = 14   # defaults to programming SPIFFS flash

    # scan thru results
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-d", "--device"):
            dev = a
        elif o in ("-S", "--sram"):
            cmmd = 15
        elif o in ("-v", "--verbose"):
            verbose = True
            
    # find device
    ldev = dev.lower()
    if ldev.startswith("usb"):
        ptype = "serial"
        port = get_serial_port_from_device(dev)
        if(port == 'none'):
            print('No ICE-V-Wireless attached to port', port)
            sys.exit(2)
    elif ldev.startswith("wifi"):
        ptype = "ip"
        port = get_IP_from_device(dev)
        pp3 = ping3.ping(port, timeout=1)
        if pp3 == False or pp3 == None:
            print('No ICE-V-Wireless found at IP', port)
            sys.exit(2)            
    elif ldev.startswith("auto"):
        ptype = "serial"
        port = get_serial_port_from_device(dev)
        if(port == 'none'):
            ptype = "ip"
            port = get_IP_from_device(dev)
            pp3 = ping3.ping(port, timeout=1)
            if pp3 == False or pp3 == None:
                print('No ICE-V-Wireless found at either USB or WIFI')
                sys.exit(2)            
    else:
        print('Unknown device', dev)
        sys.exit(2)            
    
    if verbose:
        print("Device: Type", ptype, "Port", port)

    # open chosen port
    if ptype == "serial":
        tty = serial.Serial(port)
        tty.timeout = 2 # -f option can be very slow
        
    # check for non-option arg
    if cmmd > 13:
        # bitstream file handler
        if len(args) > 0:
            send_file(args[0], cmmd, ptype, port)

            # load the FPGA with new bitstream if written to flash
            if cmmd == 14:
                load_cfg(0, ptype, port)
        else:
            print("missing filename")
    else:
        assert False, "unknown command"

    # all done
    sys.exit(0)
