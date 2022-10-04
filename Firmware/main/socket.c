/*
 * socket.c - part of esp32c3_fpga. Adds TCP socket for updating FPGA bitstream.
 * 06-22-22 E. Brombaugh
 */

#include <string.h>
#include "wifi.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "freertos/event_groups.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include "rom/crc.h"
#include "ice.h"
#include "spiffs.h"
#include "phy.h"
#include "adc_c3.h"

static const char *TAG = "socket";

#define PORT                        3333
#define KEEPALIVE_IDLE              5
#define KEEPALIVE_INTERVAL          5
#define KEEPALIVE_COUNT             3

/*
 * handle a message
 */
static void handle_message(const int sock, char *err, char cmd, char *buffer, int txsz)
{
	uint32_t Data = 0;
	char sbuf[5];
	
	if(cmd == 0xf)
	{
		/* send configuration to FPGA */
		uint8_t cfg_stat;	
		if((cfg_stat = ICE_FPGA_Config((uint8_t *)buffer, txsz)))
		{
			ESP_LOGW(TAG, "FPGA configured ERROR - status = %d", cfg_stat);
			*err |= 8;
		}
		else
			ESP_LOGI(TAG, "FPGA configured OK - status = %d", cfg_stat);
	}
	else if(cmd == 0xe)
	{
		/* save configuration to the SPIFFS filesystem */
		uint8_t cfg_stat;	
		if((cfg_stat = spiffs_write((char *)cfg_file, (uint8_t *)buffer, txsz)))
		{
			ESP_LOGW(TAG, "Config write SPIFFS Error - status = %d", cfg_stat);
			*err |= 8;
		}
		else
			ESP_LOGI(TAG, "Config write SPIFFS wrote OK - status = %d", cfg_stat);
	}
	else if(cmd == 0xc)
	{
		/* write block of data to PSRAM via SPI pass-thru */
		uint32_t Addr = *((uint32_t *)buffer);
		ESP_LOGI(TAG, "PSRAM write: Addr 0x%08X, Len 0x%08X", Addr, txsz-4);
		ICE_PSRAM_Write(Addr, (uint8_t *)buffer+4, txsz-4);
	}
	else if(cmd == 0xb)
	{
		/* read block of data from PSRAM via SPI pass-thru */
		uint32_t Addr = *((uint32_t *)buffer);
		uint32_t psram_rdsz = *((uint32_t *)(buffer+4));
#define MAX_PSRAM_RD 128		
		uint8_t psram_rdbuf[MAX_PSRAM_RD];
		
		ESP_LOGI(TAG, "PSRAM read: Addr 0x%08X, Len 0x%08X", Addr, psram_rdsz);
		
		/* Send error status */
		int written;
		while((written = send(sock, err, 1, 0)) < 1)
		{
			if(written < 0)
			{
				ESP_LOGE(TAG, "Error sending stat: errno %d", errno);
			}
		}
		
		/* Send data MAX_PSRAM_RD bytes at a time */
		while(psram_rdsz)
		{
			uint32_t rdsz = psram_rdsz > MAX_PSRAM_RD ? MAX_PSRAM_RD : psram_rdsz;
			ICE_PSRAM_Read(Addr, (uint8_t *)psram_rdbuf, rdsz);
			int to_write = rdsz;
			uint8_t *wptr = psram_rdbuf;
			while(to_write > 0)
			{
				written = send(sock, wptr, to_write, 0);
				if(written < 0)
				{
					ESP_LOGE(TAG, "Error sending data: errno %d", errno);
				}
				to_write -= written;
				wptr += written;
			}
			psram_rdsz -= rdsz;
			Addr += rdsz;
		}
	}
	else if(cmd == 0xa)
	{
		ESP_LOGW(TAG, "PSRAM Init handler in handle_message() - shouldn't get here");
		*err |= 15;
	}
	else if(cmd == 0)
	{
        /* Read SPI register */
		uint8_t Reg = *(uint32_t *)buffer & 0x7f;
		ICE_FPGA_Serial_Read(Reg, &Data);
		ESP_LOGI(TAG, "Reg read %d = 0x%08X", *(uint32_t *)buffer, Data);
	}
	else if(cmd == 1)
	{
        /* Write SPI register */
		uint8_t Reg = *(uint32_t *)buffer & 0x7f;
		Data = *(uint32_t *)&buffer[4];
		ESP_LOGI(TAG, "Reg write %d = %d", Reg, Data);
		ICE_FPGA_Serial_Write(Reg, Data);
	}
	else if(cmd == 2)
	{
        /* Report Vbat */
        Data = 2*(uint32_t)adc_c3_get();
		ESP_LOGI(TAG, "Vbat = %d mV", Data);
	}
	else if(cmd == 5)
	{
        /* Report Info */
        char infostr[64];
		infostr[0] = *err;		
		sprintf(infostr+1, "%s %s", fwVersionStr, wifi_ip_addr);
		ESP_LOGI(TAG, "Info = %s", infostr+1);
		int to_write = strlen(infostr+1)+1;
		while (to_write > 0) {
			int written = send(sock, infostr, to_write, 0);
			if (written < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			to_write -= written;
		}
	}
	else if(cmd == 6)
	{
        /* Load FPGA configuration */
		uint8_t Reg = *(uint32_t *)buffer & 0x1;
		ESP_LOGI(TAG, "Reg read %d = 0x%08X", *(uint32_t *)buffer, Data);
		const char *file = (Reg==0) ? cfg_file : spipass_file;
		load_fpga(file);
	}
	else
	{
		ESP_LOGI(TAG, "Unknown command");
		*err |= 8;
	}
	
	if((cmd == 0x0b) || (cmd == 5))
	{
		/* do nothing */
	}
	else
	{
		/* other commands are simpler */
		// send() can return less bytes than supplied length.
		// Walk-around for robust implementation.
		int to_write = ((cmd == 0) || (cmd==2)) ? 5 : 1;
		sbuf[0] = *err;
		if((cmd==0) || (cmd==2))
			memcpy(&sbuf[1], &Data, 4);
		while (to_write > 0) {
			int written = send(sock, sbuf, to_write, 0);
			if (written < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			to_write -= written;
		}
	}
	
	/* reply with error status */
	ESP_LOGI(TAG, "Reply status = %d", *err);
}

/*
 * special case for handling PSRAM Init message
 */
static void handle_ps_in(const int sock, char *err, char *left, int leftsz, int txsz)
{
	char *buffer;
	size_t act, rsz, tot, use, bufsz;
	FILE* f = NULL;
	
    /* Check if psram file exists before removing */
    struct stat st;
    if(stat(psram_file, &st) == 0)
	{
        /* Delete it if it exists */
        unlink(psram_file);
		
#if 0
		/* do SPIFFS garbage collection (not available in V4.4.2) */
		if(esp_spiffs_gc(conf.partition_label, len) != ESP_OK)
		{
			ESP_LOGE(TAG, "SPIFFS GC failed");			
			return ESP_FAIL;
		}
#endif
	}

	/* note SPIFFS avail space - only allow 75% utilization per docs */
	spiffs_info(&tot, &use);
	tot = ((4*tot)/3) - use;
	ESP_LOGI(TAG, "PS_IN: SPIFFS available: %d", tot);
	
	/* open file if enough space in SPIFFS */
	if(tot > txsz)
	{
		f = fopen(psram_file, "wb");
		if (f != NULL)
		{
			ESP_LOGI(TAG, "PS_IN: Writing %d to file %s ", txsz, psram_file);
		}
		else
		{
			ESP_LOGE(TAG, "Failed to open file for writing");
			*err = 1;
		}
	}
	else
	{
		ESP_LOGE(TAG, "Not enough space in SPIFFS - skipping.");
		*err = 1;
	}
	
	/* write initial remaining data */
	if(f)
	{
		if((act = fwrite(left, 1, leftsz, f)) != leftsz)
		{
			ESP_LOGE(TAG, "Failed writing remaining %d - actual = %d", leftsz, act);
			*err |= 2;
		}
	}
	txsz -= leftsz;
	tot = leftsz;
	
	/* loop over rest of data. Wifi maxes out at ~4kB per */
	bufsz = txsz < 4096 ? txsz : 4096;
	buffer = malloc(bufsz);
	if(buffer)
	{
		while(txsz)
		{
			/* get from socket */
			size_t bsz = txsz <  bufsz ? txsz : bufsz;
			rsz = recv(sock, buffer, bsz, 0);
			//ESP_LOGI(TAG, "Received %d", rsz);
			
			/* write to file (if open) */
			if(f)
			{
				act = fwrite(buffer, 1, rsz, f);
				if(act != rsz)
				{
					ESP_LOGE(TAG, "Failed writing %d - actual = %d", rsz, act);
					*err |= 4;
				}
			}
			
			txsz -= rsz;
			tot += rsz;
		}
		
		free(buffer);
	}
	else
	{
		ESP_LOGE(TAG, "Couldn't allocate %d", bufsz);
		*err |= 8;
	}
	
	/* close file */
	if(f)
	{
		fclose(f);
		ESP_LOGI(TAG, "Wrote %d to file %s", tot, psram_file);
	}
	else
		ESP_LOGI(TAG, "flushed %d", tot);

	/* tried flushing socket here but it hung. Doesn't seem to be needed */
	
	/* return status */
	ESP_LOGI(TAG, "PS_IN replying status");
	int to_write = 1;
	while(to_write > 0)
	{
		int written = send(sock, err, to_write, 0);
		if (written < 0)
		{
			ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
		}
		to_write -= written;
	}
	
	ESP_LOGI(TAG, "PS_IN done");
}

/*
 * receive a message
 */
static void do_getmsg(const int sock)
{
    int len, tot = 0, rxidx, sz, txsz = 0, state = 0;
    char rx_buffer[128], *filebuffer = NULL, *fptr, err=0, cmd = 0;
	union u_hdr
	{
		char bytes[8];
		unsigned int words[2];
	} header;

    do
	{
		/* get latest buffer */
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
		rxidx = 0;
		
        if(len < 0)
		{
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        }
		else if(len == 0)
		{
			if(filebuffer)
			{
				free(filebuffer);
				ESP_LOGW(TAG, "file buffer not properly freed");
			}
            ESP_LOGI(TAG, "Connection closed, tot = %d, state = %d", tot, state);
        }
		else
		{
			/* valid data - parse w/ state machine */
			int rxleft = len;
			switch(state)
			{
				case 0:
					/* waiting for magic header */
					if(tot < 8)
					{
						/* fill header buffer */
						sz = rxleft;
						sz = sz <= 8 ? sz : 8;
						memcpy(&header.bytes[tot], rx_buffer, sz);
						tot += sz;
						rxidx += sz;
						rxleft -= sz;
					}
					
					/* check if header full */
					if(tot == 8)
					{
						/* check if header matches */
						if((header.words[0] & 0xFFFFFFF0) == 0xCAFEBEE0)
						{
							cmd = header.words[0] & 0xF;
							txsz = header.words[1];
							ESP_LOGI(TAG, "State 0: Found header: cmd %1X, txsz = %d", cmd, txsz);
							
							if(cmd==0xA)
							{
								/* special case for command 0xA - PSRAM_INIT */
								if(xSemaphoreTake(ice_mutex, (TickType_t)100)==pdTRUE)
								{
									/* gather remaining */
									sz = rxleft;
									sz = sz <= txsz ? sz : txsz;
									
									/* handle the rest */
									handle_ps_in(sock, &err, rx_buffer+rxidx, sz, txsz);
								
									/* unlock resources */
									xSemaphoreGive(ice_mutex);
									
									/* advance state */
									state = 2;
								}
								else
								{
									ESP_LOGW(TAG, "Couldn't get FPGA access");
									err |= 1;
								}
							}
							else
							{
								/* all others - lock resources */
								if(xSemaphoreTake(ice_mutex, (TickType_t)100)==pdTRUE)
								{							
									/* allocate a buffer for the data */
									filebuffer = malloc(txsz);
									if(filebuffer)
									{
										/* save any remaining data in buffer */
										fptr = filebuffer;
										sz = rxleft;
										sz = sz <= txsz ? sz : txsz;
										memcpy(fptr, rx_buffer+rxidx, sz);
										//ESP_LOGI(TAG, "State 0: got %d, used %d", rxleft, sz);
										fptr += sz;
										tot += sz;
										rxleft -= sz;
									}
									else
									{
										ESP_LOGW(TAG, "Couldn't alloc buffer");
										err |= 1;
									}
								}
								else
								{
									ESP_LOGW(TAG, "Couldn't get FPGA access");
									err |= 1;
								}
								
								/* done? */
								if((tot-8)==txsz)
								{
									/* compute CRC32 to match linux crc32 cmd */
									uint32_t crc = crc32_le(0, (uint8_t *)filebuffer, txsz);
									ESP_LOGI(TAG, "State 0: Done - Received %d, CRC32 = 0x%08X", txsz, crc);
									handle_message(sock, &err, cmd, filebuffer, txsz);
									
									/* free the buffer */
									free(filebuffer);
									filebuffer = NULL;
									
									/* unlock resources */
									xSemaphoreGive(ice_mutex);
									
									/* advance to complete state */
									state = 2;
								}
								else
								{
									/* advance to next state */
									state = 1;
								}
								
								/* check for errors */
								if(rxleft)
								{
									ESP_LOGW(TAG, "Received data > txsz %d", rxleft);
									err |= 2;
								}
							}
						}
						else
						{
							ESP_LOGW(TAG, "Wrong Header 0x%08X", header.words[0]);
							err |= 4;
						}
					}
					break;
					
				case 1:
					/* collecting data in buffer */
					if(filebuffer)
					{
						int fleft = txsz-(fptr-filebuffer);
						if(fleft<txsz)
						{
							sz = len;
							sz = sz <= fleft ? sz : fleft;
							memcpy(fptr, rx_buffer, sz);
							fptr += sz;
							tot += sz;
							fleft -= sz;
							rxleft -= sz;
							//ESP_LOGI(TAG, "State 1: got %d, used %d, %d left", len, sz, fleft);
							
							/* done? */
							if((tot-8)==txsz)
							{
                                /* compute CRC32 to match linux crc32 cmd */
                                uint32_t crc = crc32_le(0, (uint8_t *)filebuffer, txsz);
								ESP_LOGI(TAG, "State 1: Done - Received %d, CRC32 = 0x%08X", txsz, crc);
								
								/* process it */
								handle_message(sock, &err, cmd, filebuffer, txsz);
								
								/* free the buffer */
								free(filebuffer);
								filebuffer = NULL;
								
								/* unlock resources */
								xSemaphoreGive(ice_mutex);
								
								/* advance state */
								state = 2;
							}
								
							/* check for errors */
							if(rxleft)
							{
								ESP_LOGW(TAG, "State 1: Received data past end", rxleft);
							}
						}
					}
					else
					{
						ESP_LOGW(TAG, "State 1: Received %d but no buffer", len);
					}
					break;
					
				case 2:
					/* still getting data */
					ESP_LOGW(TAG, "State 2 - Received data past end", rxleft);
					break;
			}
        }
    }
	while(len > 0);
}

/*
 * set up a socket and wait for activity
 */
void socket_task(void *pvParameters)
{
    char addr_str[128];
    int addr_family = (int)pvParameters;
    int ip_protocol = 0;
    int keepAlive = 1;
    int keepIdle = KEEPALIVE_IDLE;
    int keepInterval = KEEPALIVE_INTERVAL;
    int keepCount = KEEPALIVE_COUNT;
    struct sockaddr_storage dest_addr;

    if (addr_family == AF_INET) {
        struct sockaddr_in *dest_addr_ip4 = (struct sockaddr_in *)&dest_addr;
        dest_addr_ip4->sin_addr.s_addr = htonl(INADDR_ANY);
        dest_addr_ip4->sin_family = AF_INET;
        dest_addr_ip4->sin_port = htons(PORT);
        ip_protocol = IPPROTO_IP;
    }

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        vTaskDelete(NULL);
        return;
    }
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    ESP_LOGI(TAG, "Socket created");

    int err = bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        ESP_LOGE(TAG, "IPPROTO: %d", addr_family);
        goto CLEAN_UP;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto CLEAN_UP;
    }

	/* loop forever handling the socket */
    while (1) {

        ESP_LOGI(TAG, "Socket listening");

        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }

        // Set tcp keepalive option
        setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &keepIdle, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &keepInterval, sizeof(int));
        setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &keepCount, sizeof(int));
        // Convert ip address to string
        if (source_addr.ss_family == PF_INET) {
            inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr, addr_str, sizeof(addr_str) - 1);
        }
        ESP_LOGI(TAG, "Socket accepted ip address: %s", addr_str);

		/* do the thing this socket does */
        do_getmsg(sock);

        shutdown(sock, 0);
        close(sock);
    }

CLEAN_UP:
    close(listen_sock);
    vTaskDelete(NULL);
}
