/*
 * spi.c - spi port driver
 * 07-01-19 E. Brombaugh
 */

#include <stdio.h>
#include "spi.h"

/*
 * initialize spi port
 */
void spi_init(SPI_TypeDef *s)
{
	s->SPICR0 = 0xff;	// max delay counts on all auto CS timing
	s->SPICR1 = 0x84;	// enable spi, disable scsni(undocumented!)
	s->SPICR2 = 0xc0;	// master, hold cs low while busy
	s->SPIBR = 0x02;	// divide clk by 3 for spi clk
	s->SPICSR = 0x0f;	// all CS outs high
}

/*
 * send a single byte with CS
 */
void spi_tx_byte(SPI_TypeDef *s, uint8_t data)
{
	spi_cs_low(s);
	
	/* check for tx ready */
	spi_tx_wait(s);
	
	/* send data */
	s->SPITXDR = data;
	
	/* wait for rx ready (transmission complete) */
	spi_rx_wait(s);
	
	spi_cs_high(s);
}

/*
 * send & receive a single byte with CS
 */
uint8_t spi_txrx_byte(SPI_TypeDef *s, uint8_t data)
{
	spi_cs_low(s);
	
	/* check for tx ready */
	spi_tx_wait(s);
	
	/* send data */
	s->SPITXDR = data;
	
	/* wait for rx ready (transmission complete) */
	spi_rx_wait(s);
	
	spi_cs_high(s);
	
	/* get data */
	return s->SPIRXDR;
}

/*
 * send a buffer of data without CS
 */
void spi_transmit(SPI_TypeDef *s, uint8_t *src, uint16_t sz)
{
	/* loop */
	while(sz--)
	{
		/* wait for tx ready */
		spi_tx_wait(s);
	
		/* transmit a byte */
		s->SPITXDR = *src++;
	}
	
	/* wait for end of transmision */
	spi_rx_wait(s);
}

/*
 * receive a buffer of data without CS
 */
void spi_receive(SPI_TypeDef *s, uint8_t *dst, uint16_t sz)
{
	/* wait for tx ready */
	spi_tx_wait(s);
	
	/* loop */
	while(sz--)
	{
		/* send dummy data */
		s->SPITXDR = 0;
		
		/* wait for rx ready */
		spi_rx_wait(s);
		
		/* get read data */
		*dst++ = s->SPIRXDR;
	}
}