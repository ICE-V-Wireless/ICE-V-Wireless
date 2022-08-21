/*
 * uart2.c - secondary UART for debugging when primary log is disabled.
 * 08-13-22 E. Brombaugh
 */
#include "uart2.h"
#include <stdarg.h>
#include <string.h>
#include "driver/uart.h"
#include "driver/gpio.h"

#define UART2_ID 	UART_NUM_1
#define UART2_TXPIN 21	// usual logging tx pin

static int8_t uart2_enabled = 0;
static QueueHandle_t uart_queue;

/*
 * initialize secondary UART
 */
esp_err_t uart2_init(void)
{
	/* enable us */
	uart2_enabled = 1;
	
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
	const int uart_buffer_size = (1024 * 2);
	
	// Install UART driver using an event queue here
	ESP_ERROR_CHECK(uart_driver_install(UART2_ID, uart_buffer_size, \
		uart_buffer_size, 10, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(UART2_ID, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART2_ID, UART2_TXPIN, \
		UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
	
	return ESP_OK;
}

/*
 * print to secondary UART
 */
void uart2_printf(const char* format, ... )
{
    va_list args;
	char buffer[256];
	
	/* no action if not initialized */
	if(!uart2_enabled)
		return;
	
	/* print to string with varargs */
    va_start( args, format );
    vsnprintf(buffer, 256, format, args);
    va_end( args );
	
	/* send string to uart */
	uart_write_bytes(UART2_ID, (const char*)buffer, strlen(buffer));
}