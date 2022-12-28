/*
 * spi.h - SPI IP core driver
 * 07-01-19 E. Brombaugh
 */

#ifndef __spi__
#define __spi__

#include "system.h"

/* some common operation macros */
#define spi_tx_wait(s) while(!(((s)->SPISR)&0x10))
#define spi_rx_wait(s) while(!(((s)->SPISR)&0x08))
#define spi_cs_low(s) ((s)->SPICSR=0xfe)
#define spi_cs_high(s) ((s)->SPICSR=0xff)

/* spi functions */
void spi_init(SPI_TypeDef *s);
void spi_tx_byte(SPI_TypeDef *s, uint8_t data);
uint8_t spi_txrx_byte(SPI_TypeDef *s, uint8_t data);
void spi_transmit(SPI_TypeDef *s, uint8_t *src, uint16_t sz);
void spi_receive(SPI_TypeDef *s, uint8_t *dst, uint16_t sz);

#endif

