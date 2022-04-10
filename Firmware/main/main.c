/*
 * main.c - top level
 * part of lp4k_c3
 * 04-04-22 E. Brombaugh
*/

#include "main.h"
#include "ice.h"
#include "driver/gpio.h"
#include "spiffs.h"
#include "wifi.h"

#define LED_PIN 10

static const char* TAG = "lp4k_c3-main";

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
    ESP_LOGI(TAG, "LP4k Driver starting...");
    ESP_LOGI(TAG, "Version: %s", fwVersionStr);
    ESP_LOGI(TAG, "Build Date: %s", bdate);
    ESP_LOGI(TAG, "Build Time: %s", btime);

    ESP_LOGI(TAG, "Initializing SPIFFS");
	spiffs_init();

	/* init FPGA SPI port */
	ICE_Init();
    ESP_LOGI(TAG, "FPGA SPI port initialized");
	
	/* configure FPGA from SPIFFS file */
    ESP_LOGI(TAG, "Reading file %s", cfg_file);
	uint8_t *bin = NULL;
	uint32_t sz;
	if(!spiffs_read((char *)cfg_file, &bin, &sz))
	{
		/* configure FPGA from const array */
		uint8_t cfg_stat;	
		if((cfg_stat = ICE_FPGA_Config(bin, sz)))
			ESP_LOGW(TAG, "FPGA configured ERROR - status = %d", cfg_stat);
		else
			ESP_LOGI(TAG, "FPGA configured OK - status = %d", cfg_stat);
		
		free(bin);
	}
	
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
		gpio_set_level(LED_PIN, i);
		//printf("%d\n", i);
		i^=1;
		vTaskDelay(100 / portTICK_PERIOD_MS);
	}
}
