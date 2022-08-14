/*
 * uart2.h - secondary UART for debugging when primary log is disabled.
 * 08-13-22 E. Brombaugh
 */

#ifndef __UART2__
#define __UART2__

#include "main.h"

esp_err_t uart2_init(void);
void uart2_printf(const char* format, ... );

#endif

