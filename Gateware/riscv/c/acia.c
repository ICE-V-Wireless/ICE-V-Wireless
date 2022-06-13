/*
 * acia.c - serial port driver
 * 07-01-19 E. Brombaugh
 */

#include <stdio.h>
#include "acia.h"

/*
 * serial transmit character
 */
void acia_putc(char c)
{
	/* wait for tx ready */
	while(!(acia_ctlstat & 2));
	
	/* send char */
	acia_data = c;
}

/*
 * output for tiny printf
 */
void acia_printf_putc(void* p, char c)
{
	acia_putc(c);
}

/*
 * serial transmit string
 */
void acia_puts(char *str)
{
	uint8_t c;
	
	while((c=*str++))
		acia_putc(c);
}

/*
 * serial receive char
 */
int acia_getc(void)
{
	if(!(acia_ctlstat & 1))
		return EOF;
	else
		return acia_data;
}
