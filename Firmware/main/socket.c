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
	uint8_t *psram_rdbuf = NULL;
	uint32_t psram_rdsz = 0;
	
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
			ESP_LOGW(TAG, "SPIFFS Error - status = %d", cfg_stat);
			*err |= 8;
		}
		else
			ESP_LOGI(TAG, "SPIFFS wrote OK - status = %d", cfg_stat);
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
		psram_rdsz = *((uint32_t *)(buffer+4));
		psram_rdbuf = malloc(psram_rdsz+1);	// extra byte at start for err status
		if(psram_rdbuf)
		{
			ESP_LOGI(TAG, "PSRAM read: Addr 0x%08X, Len 0x%08X", Addr, psram_rdsz);
			ICE_PSRAM_Read(Addr, (uint8_t *)psram_rdbuf+1, psram_rdsz);
		}
		else
		{
			ESP_LOGW(TAG, "PSRAM read error - couldn't alloc buffer size %d", psram_rdsz+1);
			psram_rdsz = 0;
			*err |= 8;
		}
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
	else
	{
		ESP_LOGI(TAG, "Unknown command");
		*err |= 8;
	}
	
	/* reply with error status */
	ESP_LOGI(TAG, "Replying with %d", *err);
	if((cmd == 0x0b) && (psram_rdbuf))
	{
		/* PSRAM Read cmd can return a lot of data */
		// send() can return less bytes than supplied length.
		// Walk-around for robust implementation.
		int to_write = psram_rdsz+1;
		psram_rdbuf[0] = *err;	// prepend err status
		uint8_t *wbuf = psram_rdbuf;
		while (to_write > 0) {
			int written = send(sock, wbuf, to_write, 0);
			if (written < 0) {
				ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
			}
			to_write -= written;
			wbuf += written;
		}
		
		/* done with read buffer */
		free(psram_rdbuf);
		psram_rdsz = 0;
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

    do {
		/* get latest buffer */
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
		rxidx = 0;
		
        if (len < 0) {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        } else if (len == 0) {
			if(filebuffer)
			{
				free(filebuffer);
				ESP_LOGW(TAG, "file buffer not properly freed");
			}
            ESP_LOGI(TAG, "Connection closed, tot = %d, state = %d", tot, state);
        } else {
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
							
							/* lock resources */
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
    } while (len > 0);
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
