/*
 * system.h - hardware definitions for picorv32 system
 * 05-11-22 E. Brombaugh
 */

#ifndef __system__
#define __system__

#include <stdint.h>

// 32-bit parallel in / out
#define gp_out0 (*(volatile uint32_t *)0x20000000)
#define gp_out1 (*(volatile uint32_t *)0x20000004)
#define gp_out2 (*(volatile uint32_t *)0x20000008)
#define gp_out3 (*(volatile uint32_t *)0x2000000C)
#define gp_in0 (*(volatile uint32_t *)0x20000000)
#define gp_in1 (*(volatile uint32_t *)0x20000004)
#define gp_in2 (*(volatile uint32_t *)0x20000008)
#define gp_in3 (*(volatile uint32_t *)0x2000000C)

// 32-bit clock counter
#define clkcnt_reg (*(volatile uint32_t *)0x50000000)

// ACIA serial
#define acia_ctlstat (*(volatile uint8_t *)0x30000000)
#define acia_data (*(volatile uint8_t *)0x30000004)

// Mailbox
#define mbx_ctlstat (*(volatile uint8_t *)0x60000000)
#define mbx_data (*(volatile uint8_t *)0x60000004)

// SPI cores @ BUS_ADDR74 = 0b0000 and 0b0010
#define SPI0_BASE 0x40000000
#define SPI1_BASE 0x40000080

typedef struct
{
	uint32_t reserved0[8];
	volatile uint8_t SPICR0;
	uint8_t reserved1[3];
	volatile uint8_t SPICR1;
	uint8_t reserved2[3];
	volatile uint8_t SPICR2;
	uint8_t reserved3[3];
	volatile uint8_t SPIBR;
	uint8_t reserved4[3];
	volatile uint8_t SPISR;
	uint8_t reserved5[3];
	volatile uint8_t SPITXDR;
	uint8_t reserved6[3];
	volatile uint8_t SPIRXDR;
	uint8_t reserved7[3];
	volatile uint8_t SPICSR;
	uint8_t reserved8[3];
} SPI_TypeDef;

#define SPI0 ((SPI_TypeDef *) SPI0_BASE)
#define SPI1 ((SPI_TypeDef *) SPI1_BASE)

// I2C cores @ BUS_ADDR74 = 0b0001 and 0b0011
#define I2C0_BASE 0x40000040
#define I2C1_BASE 0x400000C0

typedef struct
{
	uint32_t reserved0;			// 0
	uint32_t reserved1;			// 1
	uint32_t reserved2;			// 2
	volatile uint8_t I2CSADDR;	// 3
	uint8_t reserved3[3];
	uint32_t reserved4;			// 4
	uint32_t reserved5;			// 5
	volatile uint8_t I2CIRQ;	// 6
	uint8_t reserved6[3];
	volatile uint8_t I2CIRQEN;	// 7
	uint8_t reserved7[3];
	volatile uint8_t I2CCR1;	// 8
	uint8_t reserved8[3];
	volatile uint8_t I2CCMDR;	// 9
	uint8_t reserved9[3];
	volatile uint8_t I2CBRLSB;	// A
	uint8_t reservedA[3];
	volatile uint8_t I2CBRMSB;	// B
	uint8_t reservedB[3];
	volatile uint8_t I2CSR;		// C
	uint8_t reservedC[3];
	volatile uint8_t I2CTXDR;	// D
	uint8_t reservedD[3];
	volatile uint8_t I2CRXDR;	// E
	uint8_t reservedE[3];
	volatile uint8_t I2CGCDR;	// F
	uint8_t reservedF[3];
} I2C_TypeDef;

#define I2C0 ((I2C_TypeDef *) I2C0_BASE)
#define I2C1 ((I2C_TypeDef *) I2C1_BASE)

#endif