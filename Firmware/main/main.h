/*
 * main.h - common includes for everything
 * part of lp4k_c3
 * 04-04-22 E. Brombaugh
 */

#ifndef __MAIN__
#define __MAIN__

#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

extern const char *fwVersionStr;
extern const char *cfg_file;
extern const char *spipass_file;
extern const char *psram_file;

#endif