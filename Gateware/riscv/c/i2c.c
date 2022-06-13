/*
 * i2c.c - i2c port driver
 * 06-23-20 E. Brombaugh
 */

#include <stdio.h>
#include "printf.h"
#include "clkcnt.h"
#include "i2c.h"

/* contro reg bits */
#define I2C_CCR_EN 0x80

#define I2C_CMD_STA 0x80
#define I2C_CMD_STO 0x40
#define I2C_CMD_RD 0x20
#define I2C_CMD_WR 0x10
#define I2C_CMD_ACK 0x08
#define I2C_CMD_CKSDIS 0x04

#define I2C_SR_TIP 0x80
#define I2C_SR_BUSY 0x40
#define I2C_SR_RARC 0x20
#define I2C_SR_SRW 0x10
#define I2C_SR_ARBL 0x08
#define I2C_SR_TRRDY 0x04
#define I2C_SR_TROE 0x02
#define I2C_SR_HGC 0x01

/*
 * initialize i2c port
 */
void i2c_init(I2C_TypeDef *s)
{
	s->I2CCR1 = I2C_CCR_EN | 8;	// enable I2C
	s->I2CBRMSB = 0;		// high 2 bits - resets I2C core
	s->I2CBRLSB = 60;		// low 8 bits - (24MHz/100kHz)/4 = 60
}

/*
 * i2c master transmit bytes to addr
 */
int8_t i2c_mtx(I2C_TypeDef *s, uint8_t addr, uint8_t *data, uint8_t sz)
{
	uint8_t err = 0;
	uint8_t stat;
	uint32_t timeout;
	
	/* diagnostic flag for I2C activity */
	gp_out0 |= 1;
	
	/* transmit address and no read bit */
	s->I2CTXDR = addr<<1;
	s->I2CCMDR = I2C_CMD_STA | I2C_CMD_WR | I2C_CMD_CKSDIS;

	/* loop over data */
	while(sz--)
	{
		/* wait for TRRDY */
		timeout = 3000;
		while(timeout--)
		{
			if(s->I2CSR & I2C_SR_TRRDY)
				break;
		}
		if(!timeout)
			err = 3;
		
		/* transmit data */
		s->I2CTXDR = *data++;
		s->I2CCMDR = I2C_CMD_WR | I2C_CMD_CKSDIS;
	}
	
	/* wait for TRRDY */
	timeout = 3000;
	while(timeout--)
	{
		if(s->I2CSR & I2C_SR_TRRDY)
			break;
	}
	if(!timeout)
		err = 3;
		
	/* send stop after last byte */
	s->I2CCMDR = I2C_CMD_STO | I2C_CMD_CKSDIS;
	
	/* check for error */
	stat = s->I2CSR;
	if(stat & I2C_SR_RARC)
	{
		if(stat & I2C_SR_TROE)
		{
			/* overrun + NACK */
			printf("i2c_tx: overrun error, status = %02X, resetting\n\r", s->I2CSR);
			err = 2;
			
			/* reset the core */
			s->I2CBRMSB = 0;
		}
		else
		{
			/* just a nack */
			err = 1;
		}
	}
	
	/* drop flag */
	gp_out0 &= ~1;
	
	return err;
}

/*
 * i2c slave receive
 */
int8_t i2c_srx(I2C_TypeDef *s, uint8_t addr, uint8_t *data, uint8_t sz)
{
	uint8_t err = 0, lsz = sz, rcnt = 0;
	uint32_t timeout;
	uint8_t dummy __attribute__ ((unused));
	
	/* diagnostic flag for I2C activity */
	gp_out0 |= 1;
	
	/* set slave address */
	s->I2CSADDR = addr>>2;
	s->I2CCMDR = I2C_CMD_CKSDIS;
	
	/* poll for start of transaction */
	while(!(s->I2CSR & I2C_SR_BUSY));
		
	/* flush input pipe */
	dummy = s->I2CRXDR;
	dummy = s->I2CRXDR;
	
	/* read bytes */
	while(lsz--)
	{
		/* wait for a byte */
		timeout = 3000;
		while(timeout--)
		{
			if(s->I2CSR & I2C_SR_TRRDY)
				break;
		}
		
		/* check for timeout */
		if(!timeout)
		{
			err = 1;
			break;
		}
		
		/* store data */
		*data++ = s->I2CRXDR;
		rcnt++;
		
		/* wait for TRRDY deassert */
		while(s->I2CSR & I2C_SR_TRRDY);
	}
	
	/* get what we wanted? */
	if(sz != rcnt)
		err = 1;
			
	/* drop flag */
	gp_out0 &= ~1;
	
	return err;	
}