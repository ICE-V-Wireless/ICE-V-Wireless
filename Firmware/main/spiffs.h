/*
 * spiffs.h - part of lp4k_c3. Adds spiffs file I/O
 * 04-09-22 E. Brombaugh
 */

#ifndef __SPIFFS__
#define __SPIFFS__

#include "main.h"
#include "esp_spiffs.h"

esp_err_t spiffs_init(void);
esp_err_t spiffs_info(size_t *total, size_t *used);
esp_err_t spiffs_get_fsz(char *fname, uint32_t *len);
esp_err_t spiffs_read(char *fname, uint8_t **buffer, uint32_t *len);
esp_err_t spiffs_write(char *fname, uint8_t *buffer, uint32_t len);

#endif
