/*
 * acia.h - serial port driver
 * 07-01-19 E. Brombaugh
 */

#ifndef __acia__
#define __acia__

#include "system.h"

void acia_putc(char c);
void acia_printf_putc(void* p, char c);
void acia_puts(char *str);
int acia_getc(void);

#endif

