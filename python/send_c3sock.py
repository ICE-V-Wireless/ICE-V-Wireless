#!/usr/bin/env python3
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
            print("Read Reg", reg, "=", hex(int.from_bytes(reply[1:5], byteorder='little')))
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

# send a battery command plus dummy address
def read_vbat():
    magic = make_magic(2)
    regsz = 4
    reg = 0
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
            print("Vbat =", int.from_bytes(reply[1:4], byteorder='little'), "mV")
        s.close()

# send an info command plus dummy address
def read_info():
    magic = make_magic(5)
    regsz = 4
    reg = 0
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
            rplystr = reply[1:].decode('utf-8')
            rplytok = rplystr.split()
            print("Version =", rplytok[0], ", IP Addr =", rplytok[1])
        s.close()

# write file to psram
def psram_write(cmd, psaddr, name, addr, port):
    # open file as binary
    with open(name, "rb") as file:
        # get length by seeking around
        file.seek(0, os.SEEK_END)
        file_len = file.tell()
        file.seek(0, os.SEEK_SET)
        print("Size of", name, "is", file_len, "bytes")

        # iterate over max 64kB chunks
        magic = make_magic(cmd)
        remain_len = file_len
        while remain_len:
            send_len = min(remain_len, 65536)
        
            # add the header with command
            size = send_len + 4
            size_bytes = size.to_bytes(4, byteorder = 'little')
            psaddr_bytes = psaddr.to_bytes(4, byteorder = 'little')
            payload = b"".join([magic, size_bytes, psaddr_bytes, file.read(send_len)])

            # send to the socket server on the C3
            print("psram_write: sending packet @", psaddr, " len", send_len)
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((addr, port))
                s.sendall(payload)
                reply = s.recv(1024)
                if reply[0] :
                    print("Error", reply[0])
                s.close()

            # update for next iteration
            remain_len = remain_len - send_len
            psaddr = psaddr + send_len

# read psram to stdout
def psram_read(psaddr, dlen, addr, port):
    magic = make_magic(11)
    size = 8
    size_bytes = size.to_bytes(4, byteorder = 'little')
    psaddr_bytes = psaddr.to_bytes(4, byteorder = 'little')
    dlen_bytes = dlen.to_bytes(4, byteorder = 'little')
    payload = b"".join([magic, size_bytes, psaddr_bytes, dlen_bytes])
    
    # send to the socket server on the C3
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((addr, port))
        s.sendall(payload)
        rxlen = 0
        while rxlen < dlen+1:
            reply = s.recv(65)
            if rxlen == 0:
                # check for error
                if reply[0]:
                    print("Error", reply[0])
                    return
                
                # trim off error byte
                rxlen = rxlen + 1
                reply = reply[1:]
            
            if len(reply):
                sys.stdout.buffer.write(reply)
            rxlen = rxlen + len(reply)
            
        s.close()

# send a load command plus config ID
def load_cfg(reg, addr, port):
    magic = make_magic(6)
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
        s.close()

# usage text for command line
def usage():
    print(sys.argv[0], " [options] [<file>] | [DATA] | [LEN] communicate with ESP32C3 FPGA")
    print("  -h, --help              : this message")
    print("  -a, --address=ip_addr   : address of ESP32C3 (default ICE-V.local)")
    print("  -b, --battery           : report battery voltage (in millivolts)")
    print("  -f, --flash <file>      : write <file> to SPIFFS flash")
    print("  -i, --info              : get info (version, IP addr)")
    print("  -l, --load=<cfg#>       : load config from SPIFFS (0=default, 1=spi_pass")
    print("  -p, --port=portnum      : port of FPGA load (default 3333)")
    print("  -r, --read=REG          : register to read")
    print("  -w, --write=REG DATA    : register to write and data to write")
    print("      --ps_rd=ADDR LEN    : read PSRAM at ADDR for LEN to stdout")
    print("      --ps_wr=ADDR <file> : write PSRAM at ADDR with data in <file>")
    print("      --ps_in=ADDR <file> : write PSRAM init at ADDR with data in <file>")

# main entry
if __name__ == "__main__":
    try:
        opts, args = getopt.getopt(sys.argv[1:], \
            "ha:bfil:p:r:w:", \
            ["help", "address=", "battery", "flash", "info", "load=", \
             "port=", "read=", "write=","ps_rd=", "ps_wr=", "ps_in="])
    except getopt.GetoptError as err:
        # print help information and exit:
        print(err)  # will print something like "option -a not recognized"
        usage()
        sys.exit(2)

    # defaults
    addr = "ICE-V.local"
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
        elif o in ("-b", "--battery"):
            cmmd = 2
        elif o in ("-i", "--info"):
            cmmd = 5
        elif o in ("-f", "--flash"):
            cmmd = 14
        elif o in ("-l", "--load"):
            reg = int(a) & 1
            cmmd = 6
        elif o in ("-p", "--port"):
            port = int(a)
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
        elif o in ("--ps_in"):
            cmmd = 10
            psaddr = int(a)
        else:
            assert False, "unhandled option"
    
    # check for non-option arg
    if cmmd > 13:
        # bitstream file handler
        if len(args) > 0:
            send_file(args[0], cmmd, addr, port)
        else:
            print("missing filename")
    elif (cmmd == 12) or (cmmd == 10):
        if len(args) > 0:
            psram_write(cmmd, psaddr, args[0], addr, port)
        else:
             print("missing filename")
    elif cmmd == 11:
        if len(args) > 0:
            psram_read(psaddr, int(args[0]), addr, port)
        else:
             print("missing length")
    elif cmmd == 0:
        read_reg(reg, addr, port)
    elif cmmd == 1:
        if len(args) > 0:
            write_reg(reg, int(args[0]), addr, port)
        else:
            print("missing write data")
    elif cmmd == 2:
        read_vbat()
    elif cmmd == 5:
        read_info()
    elif cmmd == 6:
        load_cfg(reg, addr, port)
    else:
        assert False, "unhandled option"



    
