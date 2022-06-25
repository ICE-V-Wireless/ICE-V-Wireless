/*
 * socket.c - part of esp32c3_fpga. Adds TCP socket for updating FPGA bitstream.
 * 06-22-22 E. Brombaugh
 */

#ifndef __SOCKET__
#define __SOCKET__

#include "main.h"

void socket_task(void *pvParameters);

#endif
