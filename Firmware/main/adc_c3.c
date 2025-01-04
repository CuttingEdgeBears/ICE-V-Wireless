/*
 * adc_c3.c - ADC driver for ESP32C3
 * 05-18-22 E. Brombaugh
 * 01-05-25 E. Brombaugh - converted to new ADC API
 */
#include "adc_c3.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define ADC_C3_UNIT ADC_UNIT_1
#define ADC_C3_CHL ADC_CHANNEL_3
#define ADC_C3_ATTEN ADC_ATTEN_DB_12

static bool adc_c3_cali_enable;
static adc_cali_handle_t adc1_cali_handle = NULL;
static adc_oneshot_unit_handle_t adc1_handle;
static const char* TAG = "adc_c3";

/*
 * from the ESP IDF examples
 */
static bool adc_calibration_init(void)
{
    esp_err_t ret;
    bool cali_enable = false;

	ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
	adc_cali_curve_fitting_config_t cali_config = {
		.unit_id = ADC_C3_UNIT,
		.chan = ADC_C3_CHL,
		.atten = ADC_C3_ATTEN,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	ret = adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle);
	if (ret != ESP_ERR_NOT_SUPPORTED)
	{
		cali_enable = true;
	}
	else
	{
		ESP_LOGW(TAG, "Calibration scheme not supported, skip software calibration");
	}

    return cali_enable;
}

/*
 * set up to read a single channel
 */
esp_err_t adc_c3_init(void)
{
	/* init ADC 1 chl 3 */
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_C3_UNIT,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_C3_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_C3_CHL, &config));

    adc_c3_cali_enable = adc_calibration_init();

    return 0;
}

/*
 * read a single channel
 */
int32_t adc_c3_get(void)
{
    int result;
	ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC_C3_CHL, &result));

    if (adc_c3_cali_enable) {
		ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, result, &result));
    }
    return result;
}