/*
 * main.c - top level
 * part of ICE-V Wireless firmware
 * 04-04-22 E. Brombaugh
*/

#include "main.h"
#include "ice.h"
#include "driver/gpio.h"
#include "rom/crc.h"
#include "spiffs.h"
#include "wifi.h"
#include "adc_c3.h"

#define LED_PIN 10

static const char* TAG = "main";

/* build version in simple format */
const char *fwVersionStr = "V0.1";
const char *cfg_file = "/spiffs/bitstream.bin";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

void app_main(void)
{
	/* Startup */
    ESP_LOGI(TAG, "-----------------------------");
    ESP_LOGI(TAG, "ICE-V Wireless starting...");
    ESP_LOGI(TAG, "Version: %s", fwVersionStr);
    ESP_LOGI(TAG, "Build Date: %s", bdate);
    ESP_LOGI(TAG, "Build Time: %s", btime);

    ESP_LOGI(TAG, "Initializing SPIFFS");
	spiffs_init();

	/* init FPGA SPI port */
	ICE_Init();
    ESP_LOGI(TAG, "FPGA SPI port initialized");
	
#if 1
	/* configure FPGA from SPIFFS file */
    ESP_LOGI(TAG, "Reading file %s", cfg_file);
	uint8_t *bin = NULL;
	uint32_t sz;
	if(!spiffs_read((char *)cfg_file, &bin, &sz))
	{
		uint8_t cfg_stat;	
#if 0
		/* configure FPGA from const array */
		if((cfg_stat = ICE_FPGA_Config(bin, sz)))
			ESP_LOGW(TAG, "FPGA configured ERROR - status = %d", cfg_stat);
		else
			ESP_LOGI(TAG, "FPGA configured OK - status = %d", cfg_stat);
#else
		/* loop on config failure */
		while((cfg_stat = ICE_FPGA_Config(bin, sz)))
			ESP_LOGW(TAG, "FPGA configured ERROR - status = %d", cfg_stat);
		ESP_LOGI(TAG, "FPGA configured OK - status = %d", cfg_stat);
#endif
		free(bin);
	}
#endif

    /* init ADC for Vbat readings */
    if(!adc_c3_init())
        ESP_LOGI(TAG, "ADC Initialized");
    else
        ESP_LOGW(TAG, "ADC Init Failed");
    
#if 1
	/* init WiFi & socket */
	if(!wifi_init())
		ESP_LOGI(TAG, "WiFi Running");
	else
		ESP_LOGE(TAG, "WiFi Init Failed");
#endif
	
	/* wait here forever and blink */
    ESP_LOGI(TAG, "Looping...", btime);
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	uint8_t i = 0;
	while(1)
	{
		gpio_set_level(LED_PIN, i&1);
		//printf("%d\n", i);
		//printf("Vbat = %d mV\n", 2*adc_c3_get());
		
		if((i&15)==0)
		{
			ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
			ESP_LOGI(TAG, "RSSI: %d", wifi_get_rssi());
		}
		
		i++;
		
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
