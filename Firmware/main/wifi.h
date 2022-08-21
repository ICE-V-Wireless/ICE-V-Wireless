/*
 * wifi.c - part of esp32c3_fpga. Set up pre-provisioned WiFi networking.
 * 04-06-22 E. Brombaugh
 */

#ifndef __WIFI__
#define __WIFI__

#include "main.h"

esp_err_t wifi_init(void);
int8_t wifi_get_rssi(void);
esp_err_t wifi_set_credentials(uint8_t cred_type, char *cred_value);

#endif
