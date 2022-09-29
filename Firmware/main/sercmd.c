/*
 * sercmd.c - USB/Serial command interface for ICE-V Wireless
 * 08-11-22 E. Brombaugh
 */
#include "sercmd.h"
#include "ice.h"
#include "spiffs.h"
#include "adc_c3.h"
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "uart2.h"
#include "rom/crc.h"
#include "esp_vfs_usb_serial_jtag.h"
#include "wifi.h"
#include "mbedtls/base64.h"

/* uncomment to turn on UART2 debugging */
//#define SERCMD_DBG

static const char* TAG = "sercmd";

static const uint8_t cmdheader[4] =
{
	0xE0, 0xBE, 0xFE, 0xCA
};

/*
 * Command handler for serial
 */
void sercmd_handle(uint8_t cmd, uint8_t *buffer, uint32_t txsz)
{
	uint32_t Data = 0;
	uint8_t err = 0, cfg_stat;	
	
	uart2_printf("sercmd_handle: cmd %d, bufsz %d\r\n", cmd, txsz);
	
	if(cmd == 0xf)
	{
		/* send configuration to FPGA */
		if((cfg_stat = ICE_FPGA_Config((uint8_t *)buffer, txsz)))
			err |= 8;
	}
	else if(cmd == 0xe)
	{
		/* save configuration to the SPIFFS filesystem */
		if((cfg_stat = spiffs_write((char *)cfg_file, (uint8_t *)buffer, txsz)))
			err |= 8;
	}
	else if(cmd == 0xc)
	{
		/* write block of data to PSRAM via SPI pass-thru */
		uint32_t Addr = *((uint32_t *)buffer);
		ICE_PSRAM_Write(Addr, (uint8_t *)buffer+4, txsz-4);
	}
	else if(cmd == 0xb)
	{
		/* read block of data from PSRAM via SPI pass-thru */
		uint32_t Addr = *((uint32_t *)buffer);
#define MAX_RDSZ 64
		uint8_t psram_rdbuf[MAX_RDSZ];
		uint32_t psram_rdsz = *((uint32_t *)(buffer+4));
		unsigned char output[2*MAX_RDSZ];
		size_t outlen;
		while(psram_rdsz)
		{
			uint32_t rdsz = psram_rdsz > MAX_RDSZ ? MAX_RDSZ : psram_rdsz;
			ICE_PSRAM_Read(Addr, (uint8_t *)psram_rdbuf, rdsz);
			mbedtls_base64_encode(output, 2*MAX_RDSZ, &outlen, psram_rdbuf, rdsz);
			output[outlen] = 0;
			uart2_printf("  RX %08X %02X %s\r\n", Addr, rdsz, output);
			fprintf(stdout, "  RX %08X %02X %s\n", Addr, rdsz, output);
			Addr += rdsz;
			psram_rdsz -= rdsz;
		}
		
		/* end condition */
		uart2_printf("  RX %08X %02X\r\n", -1, 70);
		fprintf(stdout, "  RX %08X %02X\n", -1, 70);
	}
	else if(cmd == 0xa)
	{
		/* save PSRAM data to the SPIFFS filesystem */
		if((cfg_stat = spiffs_write((char *)psram_file, (uint8_t *)buffer, txsz)))
			err |= 8;
	}
	else if(cmd == 0)
	{
        /* Read SPI register */
		uint8_t Reg = *(uint32_t *)buffer & 0x7f;
		uart2_printf("read reg %d\r\n", Reg);
		ICE_FPGA_Serial_Read(Reg, &Data);
	}
	else if(cmd == 1)
	{
        /* Write SPI register */
		uint8_t Reg = *(uint32_t *)buffer & 0x7f;
		Data = *(uint32_t *)&buffer[4];
		ICE_FPGA_Serial_Write(Reg, Data);
	}
	else if(cmd == 2)
	{
        /* Report Vbat */
        Data = 2*(uint32_t)adc_c3_get();
	}
	else if(cmd == 3)
	{
        /* Set SSID */
        if(wifi_set_credentials(0, (char *)buffer) != ESP_OK)
			err |= 1;
	}
	else if(cmd == 4)
	{
        /* Set Password */
        if(wifi_set_credentials(1, (char *)buffer) != ESP_OK)
			err |= 1;
	}
	else if(cmd == 5)
	{
        /* Report version and IP addr */
		uart2_printf("  RX %02X %s %s\r\n", err, fwVersionStr, wifi_ip_addr);
		fprintf(stdout, "  RX %02X %s %s\n", err, fwVersionStr, wifi_ip_addr);
	}
	else if(cmd == 6)
	{
        /* Load configuration */
		uint8_t Reg = *(uint32_t *)buffer & 0x1;
		uart2_printf("load cfg %d\r\n", Reg);
		const char *file = (Reg==0) ? cfg_file : spipass_file;
		load_fpga(file);
	}
	else
	{
		/* unknown command */
		err |= 8;
	}

	/* reply with error status */
	if((cmd != 0x0b) && (cmd != 5))
	{
		/* For most commands send reply as text */
		uart2_printf("short reply: RX %02X %08X\r\n", err, Data);
		fprintf(stdout, "  RX %02X %08X\n", err, Data);
	}
}

/*
 * for checking buffer contents
 */
void dump_buffer(uint8_t *buf, uint32_t sz)
{
	uint8_t *bufptr = buf;
	uint32_t i, j;
	
	/* check CRC vs linux crc32 cmd */
    uint32_t crc = crc32_le(0, buf, sz);
	uart2_printf("dump_buffer - CRC32: %08X\r\n", crc);

	/* override size */
	sz = 1024;
	
	/* loop */
	for(i=0;i<sz;i+=16)
	{
		uart2_printf("%08X ", i);
		for(j=0;j<16;j++)
		{
			uart2_printf("%02X ", *bufptr++);
		}
		uart2_printf("\r\n");
	}
}

/*
 * Task handler for serial
 */
void sercmd_task(void *pvParameters)
{
	int newchar;
	uint8_t cmdstate = 0, cmdval = 0, *buffer = NULL, *bufptr = NULL;
	uint32_t cmdsz = 0, buffsz = 0;
	
    /* Disable buffering */
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    /* Disable translation */
	esp_vfs_dev_usb_serial_jtag_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
	esp_vfs_dev_usb_serial_jtag_set_tx_line_endings(ESP_LINE_ENDINGS_LF);

    /* Enable non-blocking mode on stdin and stdout */
    fcntl(fileno(stdout), F_SETFL, 0);
    fcntl(fileno(stdin), F_SETFL, 0);

	ESP_LOGI(TAG, "Serial Command Handler listening");
	
	/* loop forever waiting for serial inputs */
    while(1)
	{
		/* character available? */
		newchar = fgetc(stdin);
		if(newchar != EOF)
		{
			if(cmdstate == 0)
			{
				/* look for first byte of header and get command */
				if((newchar & 0xf0) == (cmdheader[cmdstate] & 0xf0))
				{
					cmdval = newchar & 0x0f;
					cmdstate++;
				}
				else
					cmdstate = 0;
			}
			else if(cmdstate<4)
			{
				/* look for last 3 bytes of header */
				if(newchar == cmdheader[cmdstate])
					cmdstate++;
				else
					cmdstate = 0;
				
				if(cmdstate == 4)
					uart2_printf("header+cmd %1d\r\n", cmdval);
			}
			else if(cmdstate < 8)
			{
				/* gather the size */
				cmdsz = (cmdsz >> 8) | ((newchar & 0xff) << 24);
				cmdstate++;

				if(cmdstate == 8)
				{
					/* Got header so handle payload */
					buffsz = cmdsz;
					uart2_printf("buffsz=0x%08X\r\n", buffsz);
					
					if(buffsz)
					{
						/* lock resources */
						if(xSemaphoreTake(ice_mutex, (TickType_t)100)==pdTRUE)
						{							
							/* alloc buffer for payload */
							buffer = malloc(buffsz);
							
							if(buffer)
							{
								/* if malloc succeeded then fill the buffer */
								bufptr = buffer;
								
								/* buffer at a time - USB does 64 bytes max */
								int bytes, timeout = 0;
								while(cmdsz)
								{
									bytes = read(STDIN_FILENO, bufptr, cmdsz);
									if(bytes>0)
									{
										bufptr += bytes;
										cmdsz -= bytes;
										timeout = 0;	// reset timeout
									}
									
									/* don't hang if data ceases unexpectedly */
									if(timeout++ > 1000)
									{
										uart2_printf("timeout waiting for payload\r\n");
										cmdval = 16;	// force illegal command
										break;
									}
									
									/*
									 * was thinking of putting a vTaskDelay() here
									 * but it would likely slow things down too
									 * much.
									 */
								}
								
								//dump_buffer(buffer, buffsz);
								
								/* handle command */
								sercmd_handle(cmdval, buffer, buffsz);
								
								/* clean up */
								free(buffer);
								buffsz = 0;
								cmdstate = 0;
							}
							else
							{
								/* malloc failed - flush stdin */
								uart2_printf("malloc failed - flushing\r\n");
								int bytes, timeout = 0;
								uint8_t dummy_buf[64];
								while(cmdsz)
								{
									int fsz = cmdsz > 64 ? 64 : cmdsz;
									bytes = read(STDIN_FILENO, dummy_buf, fsz);
									if(bytes>0)
									{
										cmdsz -= bytes;
										timeout = 0;	// reset timeout
									}
									
									/* don't hang if data ceases unexpectedly */
									if(timeout++ > 1000)
									{
										uart2_printf("timeout waiting for payload\r\n");
										break;
									}
								}
								
								/* send error reply */
								fprintf(stdout, "  RX %02X %08X\n", 7, 0);
							}
							
							/* unlock resources */
							xSemaphoreGive(ice_mutex);
						}
						else
						{
							/* Someone else had the FPGA for > 1 sec */
							uart2_printf("Couldn't get FPGA access\r\n");
							cmdstate = 0;
						}
					}
					else
					{
						/* no buffer is illegal so just bail out */
						uart2_printf("No payload\r\n");
						cmdstate = 0;
					}
				}
			}
			else
				cmdstate = 0;	/* illegal state so reset */
		}
		
		/* yield to OS */
		vTaskDelay(1);
	}
	
	/* clean up - shouldn't get here */
    vTaskDelete(NULL);
}

/*
 * initialize serial command handling
 */
esp_err_t sercmd_init(void)
{
	/* turn off console logging so we can use the console for command/data */
    ESP_LOGW(TAG, "!!!Disabling Logging!!!!");
	esp_log_level_set("*", ESP_LOG_NONE);
	
	/* flush any crud out of stdout */
	fprintf(stdout, "\n\n\n");

#ifdef SERCMD_DBG
	/* init a secondary UART for debugging */
	uart2_init();
	uart2_printf("sercmd_init: start debug\r\n");
#endif
	
	/* start a separate task to monitor serial */
	if(xTaskCreate(sercmd_task, "sercmd", 4096, NULL, 5, NULL) != pdPASS)
		return ESP_FAIL;
	else
		return ESP_OK;
}
	
