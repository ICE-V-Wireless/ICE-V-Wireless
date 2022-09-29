/*
 * spiffs.c - part of lp4k_c3. Adds spiffs file I/O
 * 04-09-22 E. Brombaugh
 */

#include <string.h>
#include "spiffs.h"
#include "esp_spiffs.h"

static const char* TAG = "spiffs";

static esp_vfs_spiffs_conf_t conf =
{
  .base_path = "/spiffs",
  .partition_label = NULL,
  .max_files = 5,
  .format_if_mount_failed = true
};

/*
 * init the spiffs api
 */
esp_err_t spiffs_init(void)
{
    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
	{
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
		return ESP_FAIL;
    }
	
	ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
	return ESP_OK;
}

/*
 * get file size
 */
esp_err_t spiffs_get_fsz(char *fname, uint32_t *len)
{
	esp_err_t stat = ESP_OK;
	FILE* f = fopen(fname, "rb");
    if (f != NULL)
	{
		fseek(f, 0L, SEEK_END);
		*len = ftell(f);
		ESP_LOGI(TAG, "File size: %d", *len);
		fclose(f);
	}
	else
	{
		ESP_LOGE(TAG, "Failed to open file %s for reading", fname);
		stat = ESP_ERR_NOT_FOUND;
    }
	
	return stat;
}

/*
 * read a file into a buffer
 */
esp_err_t spiffs_read(char *fname, uint8_t **buffer, uint32_t *len)
{
	esp_err_t stat = ESP_OK;
	size_t act;
    FILE* f = fopen(fname, "rb");
    if (f != NULL)
	{
		fseek(f, 0L, SEEK_END);
		*len = ftell(f);
		ESP_LOGI(TAG, "File size: %d", *len);
		fseek(f, 0L, SEEK_SET);
		*buffer = malloc(*len);
		if(*buffer)
		{
			ESP_LOGI(TAG, "Reading %d from file %s", *len, fname);
			
			/* get data */
			if((act = fread(*buffer, 1, *len, f)) != *len)
			{
				ESP_LOGE(TAG, "Failed reading - actual = %d", act);
				stat = ESP_FAIL;
			}
		}
		else
		{
			ESP_LOGE(TAG, "Failed to malloc buffer");
			stat = ESP_ERR_NO_MEM;
		}
		fclose(f);
	}
	else
	{
		ESP_LOGE(TAG, "Failed to open file %s for reading", fname);
		stat = ESP_ERR_NOT_FOUND;
    }
	
	return stat;
}

/*
 * write a file from a buffer
 */
esp_err_t spiffs_write(char *fname, uint8_t *buffer, uint32_t len)
{
	esp_err_t stat = ESP_OK;
	size_t act;
    FILE* f = fopen(fname, "wb");
    if (f != NULL)
	{
		ESP_LOGI(TAG, "Writing %d to file %s ", len, fname);
		if((act = fwrite(buffer, 1, len, f)) != len)
		{
			ESP_LOGE(TAG, "Failed writing - actual = %d", act);
			stat = ESP_FAIL;
		}
		fclose(f);
	}
	else
	{
		ESP_LOGE(TAG, "Failed to open file for writing");
		stat = ESP_ERR_NOT_FOUND;
    }
	
	return stat;
}
