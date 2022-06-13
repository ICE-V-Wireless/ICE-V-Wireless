/*
 * psram.h - psram memory driver
 * 05-14-22 E. Brombaugh
 */

#ifndef __psram__
#define __psram__

#include "system.h"

void psram_init(SPI_TypeDef *s);
void psram_read(SPI_TypeDef *s, uint8_t *dst, uint32_t addr, uint32_t len);
void psram_write(SPI_TypeDef *s, uint8_t *src, uint32_t addr, uint32_t len);
uint16_t psram_id(SPI_TypeDef *s);

#endif

