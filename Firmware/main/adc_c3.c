/*
 * adc_c3.c - ADC driver for ESP32C3
 * 05-18-22 E. Brombaugh
 */
#include "adc_c3.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"

#define ADC_C3_CHL ADC1_CHANNEL_3
#define ADC_EXAMPLE_CALI_SCHEME     ESP_ADC_CAL_VAL_EFUSE_TP
#define ADC_EXAMPLE_ATTEN           ADC_ATTEN_DB_11

static bool adc_c3_cali_enable;
static esp_adc_cal_characteristics_t adc1_chars;
static const char* TAG = "adc_c3";

/*
 * from the ESP IDF examples
 */
static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

    ret = esp_adc_cal_check_efuse(ADC_EXAMPLE_CALI_SCHEME);
    if (ret == ESP_ERR_NOT_SUPPORTED) {
        ESP_LOGW(TAG, "Calibration scheme not supported, skip software calibration");
    } else if (ret == ESP_ERR_INVALID_VERSION) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else if (ret == ESP_OK) {
        cali_enable = true;
        esp_adc_cal_characterize(ADC_UNIT_1, ADC_EXAMPLE_ATTEN, ADC_WIDTH_BIT_DEFAULT, 0, &adc1_chars);
    } else {
        ESP_LOGE(TAG, "Invalid arg");
    }

    return cali_enable;
}

/*
 * set up to read a single channel
 */
esp_err_t adc_c3_init(void)
{
    adc_c3_cali_enable = adc_calibration_init();

    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT_DEFAULT));
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC_C3_CHL, ADC_EXAMPLE_ATTEN));

    return 0;
}

/*
 * read a single channel
 */
int32_t adc_c3_get(void)
{
    int32_t result = adc1_get_raw(ADC_C3_CHL);

    if (adc_c3_cali_enable) {
        result = esp_adc_cal_raw_to_voltage(result, &adc1_chars);
    }
    return result;
}