#!/usr/bin/python
# add a header to a file and send it to the ESP32C3 FPGA socket
# 04-07-22 E. Brombaugh

import sys
import os
import socket
import getopt

# convert a command nybble into a 32-bit magic value for the header
def make_magic(cmmd):
    return bytearray([0xE0+cmmd, 0xBE, 0xFE, 0xCA])

# send a file for direct load to FPGA or write to SPIFFS
def send_file(name, cmmd, addr, port):
    # open file as binary
    with open(name, "rb") as file:
        # get length by seeking around
        file.seek(0, os.SEEK_END)
        file_len = file.tell()
        file.seek(0, os.SEEK_SET)
        print("Size of", name, "is", file_len, "bytes")

        # add the header with command
        magic = make_magic(cmmd)
        size = file_len.to_bytes(4, byteorder = 'little')
        payload = b"".join([magic, size, file.read(file_len)])

        # send to the socket server on the C3
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((addr, port))
            s.sendall(payload)
            reply = s.recv(1024)
            if reply[0] :
                print("Error", reply[0])
            s.close()

# send a read command plus register address
def read_reg(reg, addr, port):
    magic = make_magic(0)
    regsz = 4
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send to the socket server on the C3
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((addr, port))
        s.sendall(payload)
        reply = s.recv(1024)
        if reply[0]!= 0 :
            print("Error", reply[0])
        else:
            print("Read Value", int.from_bytes(reply[1:4], byteorder='little'))
        s.close()

# send a write command plus register address and data
def write_reg(reg, data, addr, port):
    magic = make_magic(1)
    regsz = 8
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little'), data.to_bytes(4, byteorder = 'little')])
    
    # send to the socket server on the C3
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((addr, port))
        s.sendall(payload)
        reply = s.recv(1024)
        if reply[0] :
            print("Error", reply[0])
        s.close()

# usage text for command line
def usage():
    print(sys.argv[0], " [options] <file> send binary file to ESP32C3 FPGA")
    print("  -h, --help            : this message")
    print("  -a, --address=ip_addr : address of ESP32C3 (default 192.168.0.128)")
    print("  -c, --command=[0-15]  : command (default 15 = load FPGA, else write file)")
    print("  -p, --port=portnum    : port of FPGA load (default 3333)")
    print("  -r, --register=REG    : register to read/write (default 0)")

# main entry
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "ha:c:p:r:", ["help", "address=", "command=", "port="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # defaults
    addr = "192.168.0.128"
    port = 3333
    cmmd = 15
    reg = 0
    
    # scan thru results
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-a", "--addr"):
            addr = a
        elif o in ("-c", "--comm"):
            cmmd = int(a) & 15
        elif o in ("-p", "--port"):
            port = int(a)
        elif o in ("-r", "--reg"):
            reg = int(a)
        else:
            assert False, "unhandled option"
    
    # check for non-option arg
    if cmmd > 13:
        if len(args) > 0:
            send_file(args[0], cmmd, addr, port)
        else:
            print("missing filename")
    else:
        if cmmd == 0:
            read_reg(reg, addr, port)
        elif cmmd == 1:
            write_reg(reg, int(args[0]), addr, port)



    
