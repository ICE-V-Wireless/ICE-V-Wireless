/*
 * psram.c - psram memory driver
 * 05-14-22 E. Brombaugh
 */

#include "psram.h"
#include "spi.h"

/* LY68L6400 SPI commands */
/* NOTE!!! Slow read/write commands have 32-byte max burst length */
#define PSRAM_RSTEN 0x66
#define PSRAM_RST 0x99
#define PSRAM_READ 0x03
#define PSRAM_WRITE 0x02
#define PSRAM_READID 0x9F

/*
 * wake up SPI RAM
 */
void psram_init(SPI_TypeDef *s)
{
    /* Reset LY68L6400 */
	spi_tx_byte(s, PSRAM_RSTEN);
	spi_tx_byte(s, PSRAM_RST);
}

/*
 * send a header to the SPI Flash (for read/erase/write commands)
 */
void psram_header(SPI_TypeDef *s, uint8_t cmd, uint32_t addr)
{
	uint8_t txdat[4];
	
	/* assemble header */
	txdat[0] = cmd;
	txdat[1] = (addr>>16)&0xff;
	txdat[2] = (addr>>8)&0xff;
	txdat[3] = (addr)&0xff;
	
	/* send the header */
	spi_transmit(s, txdat, 4);
}

/*
 * read bytes from SPI Flash
 */
void psram_read(SPI_TypeDef *s, uint8_t *dst, uint32_t addr, uint32_t len)
{
	uint8_t dummy __attribute ((unused));
	
	spi_cs_low(s);
	
	/* send read header */
	psram_header(s, PSRAM_READ, addr);
	
	/* wait for tx ready */
	spi_tx_wait(s);
	
	/* dummy reads */
	dummy = s->SPIRXDR;
	dummy = s->SPIRXDR;
	
	/* get the buffer */
	spi_receive(s, dst, len);
	
	spi_cs_high(s);
}

/*
 * write bytes to SPI Flash
 */
void psram_write(SPI_TypeDef *s, uint8_t *src, uint32_t addr, uint32_t len)
{
	spi_cs_low(s);
	
	/* send read header */
	psram_header(s, PSRAM_WRITE, addr);
	
	/* send data packet */
	spi_transmit(s, src, len);
	
	spi_cs_high(s);
}

/*
 * get ID from SPI Flash
 */
uint16_t psram_id(SPI_TypeDef *s)
{
	uint8_t idbuf[50];
	
	spi_cs_low(s);
	
	/* send read header */
	psram_header(s, PSRAM_READID, 0);
	
	/* get the ID in our result */
	spi_receive(s, idbuf, 50);

	spi_cs_high(s);
	
	/* for some reason byte 0 is 0xFE - not per spec */
	return (idbuf[1]<<8) | idbuf[2];
}
