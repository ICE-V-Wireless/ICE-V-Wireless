/*
 * main.c - top level of picorv32 firmware
 * 06-30-19 E. Brombaugh
 */

#include <stdio.h>
#include <string.h>
#include "system.h"
#include "acia.h"
#include "printf.h"
#include "clkcnt.h"
#include "spi.h"
#include "i2c.h"
#include "psram.h"
#include "mailbox.h"

/*
 * dump a block of data
 */
void prt_blk(uint8_t *blk, uint32_t len)
{
	while(len--)
		printf("%02X, ", *blk++);
	printf("\n\r");
}

/*
 * Where it all happens. All of it.
 */
void main()
{
	/* uses stack */
	init_printf(0,acia_printf_putc);
	printf("\n\n\rup5k_esp32c3\n\r");
	
	/* initialize SPI port */
	spi_init(SPI0);
	printf("SPI0 initialized\n\r");
	
	/* initialize the PSRAM and report ID register */
	psram_init(SPI0);
	printf("PSRAM initialized: ID = 0x%04X\n\r", psram_id(SPI0));

#if 1
	/* RAM test */
	uint8_t buffer[16];
	printf("Raw buffer\n\r");
	prt_blk(buffer, 16);
	printf("writing...");
	psram_write(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	printf("reading...");
	psram_read(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	prt_blk(buffer, 16);
	
	printf("Zero buffer\n\r");
	memset(buffer, 0, 16);
	prt_blk(buffer, 16);
	printf("writing...");
	psram_write(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	printf("reading...");
	psram_read(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	prt_blk(buffer, 16);
	
	printf("AA buffer\n\r");
	memset(buffer, 0xaa, 16);
	prt_blk(buffer, 16);
	printf("writing...");
	psram_write(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	printf("reading...");
	psram_read(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	prt_blk(buffer, 16);
	
	printf("55 buffer\n\r");
	memset(buffer, 0x55, 16);
	prt_blk(buffer, 16);
	printf("writing...");
	psram_write(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	printf("reading...");
	psram_read(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	prt_blk(buffer, 16);
	
	printf("counting buffer\n\r");
	for(uint8_t i=0;i<16;i++)
		buffer[i] = i+16*i;
	prt_blk(buffer, 16);
	printf("writing...");
	psram_write(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	printf("reading...");
	psram_read(SPI0, buffer, 0, 16);
	printf("done.\n\r");
	prt_blk(buffer, 16);
#endif

#if 0
	/* test mailbox output */
	//printf("Test Mailbox:\n\r");
	for(uint8_t i=0;i<32;i++)
	{
		printf("Send %d\n\r", i);
		mailbox_putc(i);
	}
#endif

	/* counting & flashing */
	uint32_t cnt = 0;
	printf("Looping...\n\r");
	while(1)
	{		
		gp_out0 = cnt&7;
		gp_out1 = cnt&0x10 ? 0xabadcafe : 0xdeadbeef;
		printf("cnt = %d\n\r", cnt);
		cnt++;
		clkcnt_delayms(1000);
	}
}
