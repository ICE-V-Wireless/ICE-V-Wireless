#!/usr/bin/env python3
# add a header to a file and send it to the ICE-V Wireless via USB/Serial
# 08-12-22 E. Brombaugh

import sys
import os
import getopt
import time
import serial
import base64

# convert a command nybble into a 32-bit magic value for the header
def make_magic(cmmd):
    cmmd = cmmd & 15
    return bytearray([0xE0+cmmd, 0xBE, 0xFE, 0xCA])

# transmit a buffer of data to the tty
def sendall(tty, buffer):
    if 1:
        # normal operation
        size = len(buffer)
        while size:
            written = tty.write(buffer)
            size = size - written
            buffer = buffer[written:]
    else:
        # write to a file for analysis
        with open("dump.bin", "wb") as binary_file:
            binary_file.write(buffer)

# receive error and tokens
def recv_err_tokens(tty):
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

# receive a short reply with header, err status and 32-bit data as hex
def recv_err_data(tty):
    err, toks = recv_err_tokens(tty)
    if err:
        return err, 0;
    else:
        if len(toks) == 1:
            return err, int(toks[0], 16)
    
# send a file for direct load to FPGA or write to SPIFFS
def send_file(name, cmmd, tty):
    # open file as binary
    with open(name, "rb") as file:
        # get length by seeking around
        file.seek(0, os.SEEK_END)
        file_len = file.tell()
        file.seek(0, os.SEEK_SET)
        #print("Size of", name, "is", file_len, "bytes")

        # add the header with command
        magic = make_magic(cmmd)
        size = file_len.to_bytes(4, byteorder = 'little')
        payload = b"".join([magic, size, file.read(file_len)])

        # send to the C3 over usb
        sendall(tty, payload)
        err, data = recv_err_data(tty)
        if err:
            print("Error", err)
            
# send a read command plus register address
def read_reg(reg, tty):
    magic = make_magic(0)
    regsz = 4
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send to the C3 over usb
    sendall(tty, payload)
    err, data = recv_err_data(tty)
    if err:
        print("Error", err)
    else:
        print("Read Reg", reg, "=", hex(data))

# send a write command plus register address and data
def write_reg(reg, data, tty):
    magic = make_magic(1)
    regsz = 8
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little'), data.to_bytes(4, byteorder = 'little')])
    
    # send to the C3 over usb
    sendall(tty, payload)
    err, data = recv_err_data(tty)
    if err:
        print("Error", err)

# send a read vbat command
def read_vbat(tty):
    magic = make_magic(2)
    regsz = 4
    reg = 0
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send to the C3 over usb
    sendall(tty, payload)
    err, data = recv_err_data(tty)
    if err:
        print("Error", err)
    else:
        print("Vbat =", data, "mV")
    
# send a read info command
def read_info(tty):
    magic = make_magic(5)
    regsz = 4
    reg = 0
    size = regsz.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size, reg.to_bytes(4, byteorder = 'little')])
    
    # send to the C3 over usb
    sendall(tty, payload)
    err, toks = recv_err_tokens(tty)
    if err:
        print("Error", err)
    else:
        if len(toks) == 2:
            print("Version", toks[0], ", IP Addr", toks[1])
        else:
            print("Error")
    
# write file to psram
def psram_write(psaddr, name, tty):
    # open file as binary
    with open(name, "rb") as file:
        # get length by seeking around
        file.seek(0, os.SEEK_END)
        file_len = file.tell()
        file.seek(0, os.SEEK_SET)
        print("Size of", name, "is", file_len, "bytes")

        # iterate over max 64kB chunks
        magic = make_magic(12)
        remain_len = file_len
        while remain_len:
            send_len = min(remain_len, 65536)
        
            # add the header with command
            size = send_len + 4
            size_bytes = size.to_bytes(4, byteorder = 'little')
            psaddr_bytes = psaddr.to_bytes(4, byteorder = 'little')
            payload = b"".join([magic, size_bytes, psaddr_bytes, file.read(send_len)])

            # send to the C3 over usb
            print("PS_WR: sending packet @", psaddr, " len", send_len)
            sendall(tty, payload)
            err, data = recv_err_data(tty)
            if err:
                print("Error", err)

            # update for next iteration
            remain_len = remain_len - send_len
            psaddr = psaddr + send_len

# read psram to stdout
def psram_read(psaddr, numbytes, tty):
    magic = make_magic(11)
    size = 8
    size_bytes = size.to_bytes(4, byteorder = 'little')
    psaddr_bytes = psaddr.to_bytes(4, byteorder = 'little')
    len_bytes = numbytes.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size_bytes, psaddr_bytes, len_bytes])
    
    # send to the C3 over usb
    sendall(tty, payload)
    go = 1
    while go:
        reply = tty.read_until()
        #print(reply)
        rplystr = reply.decode('utf-8')
        rplytok = rplystr.split()

        # search tokens for reply header
        for tokidx in range(len(rplytok)):
            if rplytok[tokidx] == 'RX':
                # found header so return happy
                addr = int(rplytok[tokidx+1], 16)
                data_len = int(rplytok[tokidx+2], 16)
                if (addr == 0xffffffff) and (data_len == 70):
                    go = 0
                else:
                    decode_data = base64.b64decode(rplytok[tokidx+3])
                    #print("addr", addr, "data len", data_len, "data", rplytok[tokidx+3])
                    sys.stdout.buffer.write(decode_data)
                break
            
        if tokidx == len(rplytok):
            # didn't find header
            print("No header")
            go = 0
    return

# send credentials (ssid or password)
def send_cred(cred_type, cred_value, tty):
    cred_type = cred_type & 1
    magic = make_magic(3+cred_type)
    size = len(cred_value)+1
    size_bytes = size.to_bytes(4, byteorder = 'little')
    cred_value_bytes = bytes(cred_value, 'ascii') + b'\x00'
    payload = b"".join([magic, size_bytes, cred_value_bytes])
    
    # send to the C3 over usb
    sendall(tty, payload)
    err, data = recv_err_data(tty)
    if err:
        print("Error", err)
    
    
# usage text for command line
def usage():
    print(sys.argv[0], " [options] [<file>] | [DATA] | [LEN] communicate with ESP32C3 FPGA")
    print("  -h, --help              : this message")
    print("  -p, --port=<tty>        : usb tty of ESP32C3 (default /dev/ttyACM0)")
    print("  -b, --battery           : report battery voltage (in millivolts)")
    print("  -f, --flash=<file>      : write <file> to SPIFFS flash")
    print("  -i, --info              : get info (version, IP addr)")
    print("  -r, --read=REG          : register to read")
    print("  -w, --write=REG DATA    : register to write and data to write")
    print("      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout")
    print("      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>")
    print("  -s, --ssid <SSID>       : set WiFi SSID")
    print("  -o, --password <pwd>    : set WiFi Password")

# main entry
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], \
            "hp:bfir:w:so", \
            ["help", "port=", "battery", "flash", "info", "read=", "write=", \
             "ps_rd=", "ps_wr=", "ssid", "password"])
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
        elif o in ("-i", "--info"):
            cmmd = 5
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
        elif o in ("-s", "--ssid"):
            cmmd = 3
        elif o in ("-o", "--password"):
            cmmd = 4
        else:
            assert False, "unhandled option"

    # try to open the port and run the command
    tty = serial.Serial(port)
    tty.timeout = 2 # -f option can be very slow

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
        if 1:
            if len(args) > 0:
                psram_read(psaddr, int(args[0]), tty)
            else:
                 print("missing length")
        else:
            print("Not yet supported")
    elif cmmd == 0:
        read_reg(reg, tty)
    elif cmmd == 1:
        if len(args) > 0:
            write_reg(reg, int(args[0]), tty)
        else:
            print("missing write data")
    elif cmmd == 2:
        read_vbat(tty)
    elif cmmd == 3:
        send_cred(0, args[0], tty)
    elif cmmd == 4:
        send_cred(1, args[0], tty)
    elif cmmd == 5:
        read_info(tty)
    else:
        assert False, "unknown command"
       

    
