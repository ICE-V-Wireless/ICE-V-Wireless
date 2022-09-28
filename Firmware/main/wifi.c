/*
 * wifi.c - part of esp32c3_fpga. Set up pre-provisioned WiFi networking.
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
#include "phy.h"
#include "socket.h"
#include "mdns.h"
#include "esp_idf_version.h"
#include "uart2.h"

static const char *TAG = "wifi";
#define DEFAULT_WIFI_SSID "MY_SSID"
#define DEFAULT_WIFI_PASSWORD "MY_PASSWORD"

/******************************************************************************/
/* Basic WiFi connection                                                      */
/******************************************************************************/
static int s_active_interfaces = 0;
static xSemaphoreHandle s_semph_get_ip_addrs;
static esp_netif_t *s_example_esp_netif = NULL;

char wifi_ip_addr[32];

#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(4, 4, 2)
// there's an include for this in V4.4.2 and beyond
extern void phy_bbpll_en_usb(bool en);
#endif

/* stuff that's usually in the menuconfig */
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

/*
 * start WiFi up with stored credentials
 */
static esp_netif_t *wifi_start(void)
{
	esp_err_t err;
    char *desc;
	
	/* init wifi */
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
            .ssid = DEFAULT_WIFI_SSID,
            .password = DEFAULT_WIFI_PASSWORD,
            .scan_method = EXAMPLE_WIFI_SCAN_METHOD,
            .sort_method = EXAMPLE_WIFI_CONNECT_AP_SORT_METHOD,
            .threshold.rssi = CONFIG_EXAMPLE_WIFI_SCAN_RSSI_THRESHOLD,
            .threshold.authmode = EXAMPLE_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };
	
	/* set credentials */
	nvs_handle_t my_handle;
    err = nvs_open("credentials", NVS_READWRITE, &my_handle);
    if(err != ESP_OK)
	{
        ESP_LOGW(TAG, "wifi_start: Error (%s) opening NVS", esp_err_to_name(err));
    }
	else
	{
		size_t cred_len;
		uint8_t commit = 0;
		
		/* get SSID */
		cred_len = 32; 	// size of wifi_config.sta.ssid array
		err = nvs_get_blob(my_handle, "ssid", wifi_config.sta.ssid, &cred_len);
		if(err == ESP_ERR_NVS_NOT_FOUND)
		{
			err = nvs_set_blob(my_handle, "ssid", DEFAULT_WIFI_SSID, cred_len);
			ESP_LOGI(TAG, "Created default ssid = %s, err = %s", DEFAULT_WIFI_SSID, esp_err_to_name(err));
			commit = 1;
		}
		else if(err != ESP_OK)
		{
			ESP_LOGE(TAG, "Error getting ssid: %s", esp_err_to_name(err));
		}
		else
			ESP_LOGI(TAG, "Found ssid = %s", wifi_config.sta.ssid);
		
		/* get password */
		cred_len = 64; 	// size of wifi_config.sta.password array
		err = nvs_get_blob(my_handle, "password", wifi_config.sta.password, &cred_len);
		if(err == ESP_ERR_NVS_NOT_FOUND)
		{
			err = nvs_set_blob(my_handle, "password", DEFAULT_WIFI_PASSWORD, cred_len);
			ESP_LOGI(TAG, "Created default password = %s, err = %s", DEFAULT_WIFI_PASSWORD, esp_err_to_name(err));
			commit = 1;
		}
		else if(err != ESP_OK)
		{
			ESP_LOGE(TAG, "Unknown error getting password: %s", esp_err_to_name(err));
		}
		else
			ESP_LOGI(TAG, "Found password");
		
		if(commit)
		{
			err = nvs_commit(my_handle);
			ESP_LOGI(TAG, "commit: err = %s", esp_err_to_name(err));
		}
		
		nvs_close(my_handle);
	}
	
	/* spritetm's workaround for auto USB disable - does this work? */
	ESP_LOGI(TAG, "Preventing USB disable");
	phy_bbpll_en_usb(true);
	
	/* only attempt connect if not default credentials */
    if(strncmp((char *)wifi_config.sta.ssid, DEFAULT_WIFI_SSID, 32) && 
		strncmp((char *)wifi_config.sta.password, DEFAULT_WIFI_PASSWORD, 64))
	{
		ESP_LOGI(TAG, "Connecting to %s...", wifi_config.sta.ssid);
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
		ESP_ERROR_CHECK(esp_wifi_start());
		
		esp_wifi_connect();
		
		return netif;
	}
	else
	{
		ESP_LOGI(TAG, "Default Credentials Detected - Not Connecting");
		return NULL;
	}
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
    if(s_example_esp_netif)
	{
		s_active_interfaces++;

		/* create semaphore if at least one interface is active */
		s_semph_get_ip_addrs = xSemaphoreCreateCounting(NR_OF_IP_ADDRESSES_TO_WAIT_FOR, 0);
	}
}

/*
 * tear down connection, release resources
 */
static void stop(void)
{
    wifi_stop();
}

/*
 * manage network startup for wifi, with timeout
 */
esp_err_t example_connect(void)
{
    if (s_semph_get_ip_addrs != NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    start();
    if(s_active_interfaces)
	{
		ESP_ERROR_CHECK(esp_register_shutdown_handler(&stop));
		ESP_LOGI(TAG, "Waiting for IP(s)");
		for (int i = 0; i < NR_OF_IP_ADDRESSES_TO_WAIT_FOR; ++i)
		{
			/* wait up to 10 sec for WiFi to connect */
			if(xSemaphoreTake(s_semph_get_ip_addrs, 10000/portTICK_PERIOD_MS) != pdTRUE)
			{
				ESP_LOGW(TAG, "Failed to connect");
				stop();
				return ESP_FAIL;
			}
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
				sprintf(wifi_ip_addr, IPSTR, IP2STR(&ip.ip));
			}
		}
		return ESP_OK;
	}
	else
		return ESP_FAIL;
}

/*
 * initialize the WiFi interface
 */
esp_err_t wifi_init(void)
{
	/* default wifi IP */
	strcpy(wifi_ip_addr, "unavailable");
	
	/* minimum stuff needed for IPV4 WiFi connection */
    ESP_ERROR_CHECK(nvs_flash_init());
	ESP_ERROR_CHECK(esp_netif_init());
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	if(example_connect() == ESP_OK)
	{
		//initialize mDNS service
		ESP_ERROR_CHECK( mdns_init() );
		ESP_ERROR_CHECK( mdns_hostname_set("ICE-V") );
		ESP_ERROR_CHECK( mdns_instance_name_set("ESP32C3 + FPGA") );
		ESP_ERROR_CHECK( mdns_service_add(NULL, "_FPGA", "_tcp", 3333, NULL, 0)  );
		
		/* whatever else you want running on top of WiFi */
		xTaskCreate(socket_task, "socket", 4096, (void*)AF_INET, 5, NULL);
		
		return ESP_OK;
	}
	else
		return ESP_FAIL;
}

/*
 * get RSSI
 */
int8_t wifi_get_rssi(void)
{
	wifi_ap_record_t ap;
	esp_wifi_sta_get_ap_info(&ap);
	return ap.rssi;
}

/*
 * update credentials in nvs
 */
esp_err_t wifi_set_credentials(uint8_t cred_type, char *cred_value)
{
	if(cred_type > 1)
	{
		uart2_printf("wifi_set_credentials: illegal credential type %d\r\n",
			cred_type);
		return ESP_FAIL;
	}
	
	size_t cred_len = cred_type ? 64 : 32;
	if((strlen(cred_value)+1) >= cred_len)
	{
        uart2_printf("wifi_set_credentials: illegal credential length %d\r\n",
			(strlen(cred_value)+1));
		return ESP_FAIL;
	}
	
	nvs_handle_t my_handle;
	esp_err_t err;
    if((err = nvs_open("credentials", NVS_READWRITE, &my_handle)) != ESP_OK)
	{
        uart2_printf("wifi_set_credentials: Error (%s) opening NVS\r\n",
			esp_err_to_name(err));
		return ESP_FAIL;
    }
		
	char *cred_key = cred_type ? "password" : "ssid";
	if((err = nvs_set_blob(my_handle, cred_key, cred_value, cred_len)) == ESP_OK)
	{
		uart2_printf("wifi_set_credentials: Wrote %s = %s, err = %s\r\n",
			cred_key, cred_value, esp_err_to_name(err));
		err = nvs_commit(my_handle);
		uart2_printf("wifi_set_credentials: commit: err = %s\r\n", esp_err_to_name(err));
	}
	
	nvs_close(my_handle);

	return ESP_OK;
}