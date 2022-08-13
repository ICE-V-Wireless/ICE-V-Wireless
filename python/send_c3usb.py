#!/usr/bin/python
# add a header to a file and send it to the ESP32C3 FPGA USB/Serial
# 08-12-22 E. Brombaugh

import sys
import os
import io
import getopt

# convert a command nybble into a 32-bit magic value for the header
def make_magic(cmmd):
    return bytearray([0xE0+cmmd, 0xBE, 0xFE, 0xCA])

# transmit a buffer of data to the tty
def sendall(tty, buffer):
    size = len(buffer)
    print(f"start size = {size}")
    while size:
        written = tty.write(buffer)
        size = size - written
        buffer = buffer[written:]
        print(f"write size = {size}")

# receive a buffer of data from the tty
def recv(tty, lenbytes):
    return tty.read(lenbytes)

# send a file for direct load to FPGA or write to SPIFFS
def send_file(name, cmmd, tty):
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
        sendall(tty, payload)
        reply = recv(tty, 1024)
        if reply[0] :
            print("Error", reply[0])

# send a read command plus register address
def read_reg(reg, addr, tty):
    magic = make_magic(0)
    regsz = 4
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send to the socket server on the C3
    sendall(tty, payload)
    reply = recv(tty, 1024)
    if reply[0]!= 0 :
        print("Error", reply[0])
    else:
        print("Read Reg", reg, "=", hex(int.from_bytes(reply[1:5], byteorder='little')))

# send a write command plus register address and data
def write_reg(reg, data, addr, tty):
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

# send a read command plus register address
def read_vbat(tty):
    magic = make_magic(2)
    regsz = 4
    reg = 0
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send to the socket server on the C3
    sendall(tty, payload)
    reply = recv(tty, 1024)
    if reply[0]!= 0 :
        print("Error", reply[0])
    else:
        print("Vbat =", int.from_bytes(reply[1:4], byteorder='little'), "mV")

# write file to psram
def psram_write(psaddr, name, tty):
    # open file as binary
    with open(name, "rb") as file:
        # get length by seeking around
        file.seek(0, os.SEEK_END)
        file_len = file.tell()
        file.seek(0, os.SEEK_SET)
        print("Size of", name, "is", file_len, "bytes")

        # add the header with command
        magic = make_magic(12)
        size = file_len + 4
        size_bytes = size.to_bytes(4, byteorder = 'little')
        psaddr_bytes = psaddr.to_bytes(4, byteorder = 'little')
        payload = b"".join([magic, size_bytes, psaddr_bytes, file.read(file_len)])

        # send to the socket server on the C3
        sendall(tty, payload)
        reply = recv(tty, 1024)
        if reply[0] :
            print("Error", reply[0])

# read psram to stdout
def psram_read(psaddr, numbytes, tty):
    magic = make_magic(11)
    size = 8
    size_bytes = size.to_bytes(4, byteorder = 'little')
    psaddr_bytes = psaddr.to_bytes(4, byteorder = 'little')
    len_bytes = numbytes.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size_bytes, psaddr_bytes, len_bytes])
    
    # send to the socket server on the C3
    sendall(tty, payload)
    reply = recv(tty, len+1)
    if reply[0] :
        print("Error", reply[0])
    sys.stdout.buffer.write(reply[1:])

# usage text for command line
def usage():
    print(sys.argv[0], " [options] [<file>] | [DATA] | [LEN] communicate with ESP32C3 FPGA")
    print("  -h, --help              : this message")
    print("  -p, --port=<tty>        : usb tty of ESP32C3 (default /dev/ttyACM0)")
    print("  -b, --battery           : report battery voltage (in millivolts)")
    print("  -f, --flash=<file>      : write <file> to SPIFFS flash")
    print("  -r, --read=REG          : register to read")
    print("  -w, --write=REG DATA    : register to write and data to write")
    print("      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout")
    print("      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>")

# main entry
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], "hp:bfr:w:", ["help", "port=", "battery", "flash", "read=", "write=","ps_rd=", "ps_wr="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # defaults
    port = "/dev/ttyACM0"
    cmmd = 15
    reg = 0
    
    # scan thru results
    for o, a in opts:
        if o in ("-h", "--help"):
            usage()
            sys.exit()
        elif o in ("-p", "--port"):
            port = a
        elif o in ("-b", "--battery"):
            cmmd = 2
        elif o in ("-f", "--flash"):
            cmmd = 14
        elif o in ("-r", "--read"):
            reg = int(a)
            cmmd = 0
        elif o in ("-w", "--write"):
            reg = int(a)
            cmmd = 1
        elif o in ("--ps_rd"):
            cmmd = 11
            psaddr = int(a)
        elif o in ("--ps_wr"):
            cmmd = 12
            psaddr = int(a)
        else:
            assert False, "unhandled option"

    # try to open the port and run the command
    fd = os.open(port, os.O_RDWR | os.O_NOCTTY)
    tty = io.FileIO(fd, 'w+')

    # flush any RX data from the tty
    #while tty.readable():
     #   junk = tty.read(-1)

    # check for non-option arg
    if cmmd > 13:
        # bitstream file handler
        if len(args) > 0:
            send_file(args[0], cmmd, tty)
        else:
            print("missing filename")
    elif cmmd == 12:
        if len(args) > 0:
            psram_write(psaddr, args[0], tty)
        else:
             print("missing filename")
    elif cmmd == 11:
        if len(args) > 0:
            psram_read(psaddr, int(args[0]), tty)
        else:
             print("missing length")
    elif cmmd == 0:
        read_reg(reg, tty)
    elif cmmd == 1:
        if len(args) > 0:
            write_reg(reg, int(args[0]), tty)
        else:
            print("missing write data")
    elif cmmd == 2:
        read_vbat(tty)
    else:
        assert False, "unknown command"
       

    
