/*
 * mailbox.c - fifo port driver
 * 06-04-22 E. Brombaugh
 */

#include <stdio.h>
#include "mailbox.h"

/*
 * mailbox transmit character
 */
void mailbox_putc(char c)
{
	/* wait for not full */
	while((mbx_ctlstat & 2));
	
	/* send char */
	mbx_data = c;
}

/*
 * output for tiny printf
 */
void mailbox_printf_putc(void* p, char c)
{
	mailbox_putc(c);
}

/*
 * mailbox transmit string
 */
void mailbox_puts(char *str)
{
	uint8_t c;
	
	while((c=*str++))
		mailbox_putc(c);
}

/*
 * mailbox receive char 
 */
int mailbox_getc(void)
{
	if((mbx_ctlstat & 1))
		return EOF;
	else
		return mbx_data;
}
