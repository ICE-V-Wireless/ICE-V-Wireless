/*
 * wifi.c - part of lp4k_c3. Adds TCP socket for updating FPGA bitstream
 * 04-06-22 E. Brombaugh
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
#include "ice.h"
#include "spiffs.h"
#include "phy.h"

static const char *TAG = "wifi";

/******************************************************************************/
/* Basic WiFi connection                                                      */
/******************************************************************************/
static int s_active_interfaces = 0;
static xSemaphoreHandle s_semph_get_ip_addrs;
static esp_netif_t *s_example_esp_netif = NULL;

// there's an include for this but it doesn't define the function
// if it doesn't think it needs it, so manually declare the function
//extern void phy_bbpll_en_usb(bool en);

/* stuff that's usually in the menuconfig */
#define CONFIG_EXAMPLE_WIFI_SSID "dummy"
#define CONFIG_EXAMPLE_WIFI_PASSWORD "dummy"
#define CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD -127
#define EXAMPLE_WIFI_SCAN_METHOD WIFI_ALL_CHANNEL_SCAN
#define EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#define EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD WIFI_CONNECT_AP_BY_SIGNAL
#define NR_OF_IP_ADDRESSES_TO_WAIT_FOR (s_active_interfaces)

esp_netif_t *get_example_netif_from_desc(const char *desc)
{
    esp_netif_t *netif = NULL;
    char *expected_desc;
    asprintf(&expected_desc, "%s: %s", TAG, desc);
    while ((netif = esp_netif_next(netif)) != NULL) {
        if (strcmp(esp_netif_get_desc(netif), expected_desc) == 0) {
            free(expected_desc);
            return netif;
        }
    }
    free(expected_desc);
    return netif;
}

/**
 * @brief Checks the netif description if it contains specified prefix.
 * All netifs created withing common connect component are prefixed with the module TAG,
 * so it returns true if the specified netif is owned by this module
 */
static bool is_our_netif(const char *prefix, esp_netif_t *netif)
{
    return strncmp(prefix, esp_netif_get_desc(netif), strlen(prefix) - 1) == 0;
}

static esp_ip4_addr_t s_ip_addr;

static void on_got_ip(void *arg, esp_event_base_t event_base,
                      int32_t event_id, void *event_data)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    if (!is_our_netif(TAG, event->esp_netif)) {
        ESP_LOGW(TAG, "Got IPv4 from another interface \"%s\": ignored", esp_netif_get_desc(event->esp_netif));
        return;
    }
    ESP_LOGI(TAG, "Got IPv4 event: Interface \"%s\" address: " IPSTR, esp_netif_get_desc(event->esp_netif), IP2STR(&event->ip_info.ip));
    memcpy(&s_ip_addr, &event->ip_info.ip, sizeof(s_ip_addr));
    xSemaphoreGive(s_semph_get_ip_addrs);
}

static void on_wifi_disconnect(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    ESP_LOGI(TAG, "Wi-Fi disconnected, trying to reconnect...");
    esp_err_t err = esp_wifi_connect();
    if (err == ESP_ERR_WIFI_NOT_STARTED) {
        return;
    }
    ESP_ERROR_CHECK(err);
}

static esp_netif_t *wifi_start(void)
{
    char *desc;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_netif_inherent_config_t esp_netif_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    // Prefix the interface description with the module TAG
    // Warning: the interface desc is used in tests to capture actual connection details (IP, gw, mask)
    asprintf(&desc, "%s: %s", TAG, esp_netif_config.if_desc);
    esp_netif_config.if_desc = desc;
    esp_netif_config.route_prio = 128;
    esp_netif_t *netif = esp_netif_create_wifi(WIFI_IF_STA, &esp_netif_config);
    free(desc);
    esp_wifi_set_default_wifi_sta_handlers();

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip, NULL));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_EXAMPLE_WIFI_SSID,
            .password = CONFIG_EXAMPLE_WIFI_PASSWORD,
            .scan_method = EXAMPLE_WIFI_SCAN_METHOD,
            .sort_method = EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
	
	/* spritetm's workaround for auto USB disable - does this work? */
	ESP_LOGI(TAG, "Attempting to re-enable USB");
	phy_bbpll_en_usb(true);

    ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
	
    esp_wifi_connect();

    return netif;
}

static void wifi_stop(void)
{
    esp_netif_t *wifi_netif = get_example_netif_from_desc("sta");
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &on_wifi_disconnect));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &on_got_ip));
    esp_err_t err = esp_wifi_stop();
    if (err == ESP_ERR_WIFI_NOT_INIT) {
        return;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(wifi_netif));
    esp_netif_destroy(wifi_netif);
    s_example_esp_netif = NULL;
}

/* set up connection, Wi-Fi and/or Ethernet */
static void start(void)
{
    s_example_esp_netif = wifi_start();
    s_active_interfaces++;

    /* create semaphore if at least one interface is active */
    s_semph_get_ip_addrs = xSemaphoreCreateCounting(NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0);
}

/* tear down connection, release resources */
static void stop(void)
{
    wifi_stop();
}

esp_err_t example_connect(void)
{
    if (s_semph_get_ip_addrs != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    start();
    ESP_ERROR_CHECK(esp_register_shutdown_handler(&stop));
    ESP_LOGI(TAG, "Waiting for IP(s)");
    for (int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i) {
        xSemaphoreTake(s_semph_get_ip_addrs, portMAX_DELAY);
    }
    // iterate over active interfaces, and print out IPs of "our" netifs
    esp_netif_t *netif = NULL;
    esp_netif_ip_info_t ip;
    for (int i = 0; i < esp_netif_get_nr_of_ifs(); ++i) {
        netif = esp_netif_next(netif);
        if (is_our_netif(TAG, netif)) {
            ESP_LOGI(TAG, "Connected to %s", esp_netif_get_desc(netif));
            ESP_ERROR_CHECK(esp_netif_get_ip_info(netif, &ip));

            ESP_LOGI(TAG, "- IPv4 address: " IPSTR, IP2STR(&ip.ip));
        }
    }
    return ESP_OK;
}

/******************************************************************************/
/* Socket stuff                                                               */
/******************************************************************************/
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
		/* send to FPGA */
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
		/* save to the SPIFFS filesystem */
		uint8_t cfg_stat;	
		if((cfg_stat = spiffs_write((char *)cfg_file, (uint8_t *)buffer, txsz)))
		{
			ESP_LOGW(TAG, "SPIFFS Error - status = %d", cfg_stat);
			*err |= 8;
		}
		else
			ESP_LOGI(TAG, "SPIFFS wrote OK - status = %d", cfg_stat);
	}
	else if(cmd == 0)
	{
		uint8_t Reg = *(uint32_t *)buffer & 0x7f;
		ICE_FPGA_Serial_Read(Reg, &Data);
		ESP_LOGI(TAG, "Reg read %d = 0x%08X", *(uint32_t *)buffer, Data);
	}
	else if(cmd == 1)
	{
		uint8_t Reg = *(uint32_t *)buffer & 0x7f;
		Data = *(uint32_t *)&buffer[4];
		ESP_LOGI(TAG, "Reg write %d = %d", Reg, Data);
		ICE_FPGA_Serial_Write(Reg, Data);
	}
	else
	{
		ESP_LOGI(TAG, "Unknown command");
		*err |= 8;
	}
	
	/* reply with error status */
	ESP_LOGI(TAG, "Replying with %d", *err);
	// send() can return less bytes than supplied length.
	// Walk-around for robust implementation.
	int to_write = cmd == 0 ? 5 : 1;
	sbuf[0] = *err;
	if(cmd==0)
		memcpy(&sbuf[1], &Data, 4);
	while (to_write > 0) {
		int written = send(sock, sbuf, to_write, 0);
		if (written < 0) {
			ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
		}
		to_write -= written;
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
							
							/* done? */
							if((tot-8)==txsz)
							{
								ESP_LOGI(TAG, "State 0: Done - Received %d", txsz);
								handle_message(sock, &err, cmd, filebuffer, txsz);
								
								/* free the buffer */
								free(filebuffer);
								filebuffer = NULL;

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
								ESP_LOGI(TAG, "State 1: Done - Received %d", txsz);
								
								/* process it */
								handle_message(sock, &err, cmd, filebuffer, txsz);
								
								/* free the buffer */
								free(filebuffer);
								filebuffer = NULL;
								
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
static void tcp_server_task(void *pvParameters)
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

/******************************************************************************/
/* API                                                                        */
/******************************************************************************/
/*
 * initialize the WiFi interface
 */
esp_err_t wifi_init(void)
{
	esp_err_t ret = ESP_OK;
	
	/* minimum stuff needed for IPV4 WiFi connection */
    ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(example_connect());
	
	/* whatever else you want running on top of WiFi */
	xTaskCreate(tcp_server_task, "tcp_server", 4096, (void*)AF_INET, 5, NULL);
	return ret;
}

