/*
 * mailbox.h - fifo port driver
 * 06-04-22 E. Brombaugh
 */

#ifndef __mailbox__
#define __mailbox__

#include "system.h"

void mailbox_putc(char c);
void mailbox_printf_putc(void* p, char c);
void mailbox_puts(char *str);
int mailbox_getc(void);

#endif

