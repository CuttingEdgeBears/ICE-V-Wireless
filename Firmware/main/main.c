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
#include "sercmd.h"

#define LED_PIN 10

static const char* TAG = "main";

/* build version in simple format */
const char *fwVersionStr = "0.4";
const char *cfg_file = "/spiffs/bitstream.bin";
const char *spipass_file = "/spiffs/spi_pass.bin";
const char *psram_file = "/spiffs/psram.bin";

/* build time */
const char *bdate = __DATE__;
const char *btime = __TIME__;

/*
 * common FPGA file loader
 */
esp_err_t load_fpga(const char *filename)
{
	uint8_t *bin = NULL;
	uint32_t sz;
	
	ESP_LOGI(TAG, "Configuring FPGA from file %s", filename);
	if(!spiffs_read((char *)filename, &bin, &sz))
	{
		uint8_t cfg_stat;
		
		/* loop on config failure */
		int8_t retry = 4;
		while((cfg_stat = ICE_FPGA_Config(bin, sz)) && (retry--))
			ESP_LOGW(TAG, "FPGA configured ERROR - status = %d", cfg_stat);
		if(retry)
			ESP_LOGI(TAG, "FPGA configured OK - status = %d", cfg_stat);
		else
			ESP_LOGW(TAG, "FPGA configured ERROR - giving up");

		free(bin);
		
		return ESP_OK;
	}
	
	ESP_LOGI(TAG, "Configuration file %s not found", filename);
	return ESP_FAIL;
}

/*
 * entry point
 */
void app_main(void)
{
	uint32_t blink_period = 500;
	uint32_t sz;
	
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
	
	/* preload PSRAM */
    ESP_LOGI(TAG, "Pre-Loading PSRAM from file %s", psram_file);
	if(!spiffs_get_fsz((char *)psram_file, &sz))
	{
		if(sz > 4)
		{
			/* preload FPGA with SPI Pass-thru design */
			load_fpga(spipass_file);
			
			/* Get data from file and send */
			{
				size_t act;
				FILE* f = fopen(psram_file, "rb");
				if (f != NULL)
				{
					/* get size */
					fseek(f, 0L, SEEK_END);
					sz = ftell(f);
					fseek(f, 0L, SEEK_SET);
					
					/* set up a big read buffer */
					uint8_t *buffer;
					size_t bufsz = sz < 65536 ? sz : 65536;
					buffer = malloc(bufsz);
					
					/* get starting offset */
					uint32_t Addr;
					act = fread(&Addr, 1, sizeof(uint32_t), f);
					sz -= 4;
					ESP_LOGI(TAG, "PSRAM write: Addr 0x%08"PRIx32", Len 0x%08"PRIx32"", Addr, sz);

					/* read from file and send to PSRAM */
					esp_err_t err = ESP_OK;
					while(sz)
					{
						size_t rsz = sz < bufsz ? sz : bufsz;
						act = fread(buffer, 1, rsz, f);
						if(act != rsz)
						{
							ESP_LOGE(TAG, "Failed reading %d, actual = %d", rsz, act);
							err = ESP_FAIL;
							break;
						}
						else
						{
							ICE_PSRAM_Write(Addr, buffer, rsz);
							ESP_LOGI(TAG, "  chunk @ Addr 0x%08"PRIx32", Len 0x%08X", Addr, rsz);
						}
						sz -= rsz;
						Addr += rsz;
					}
					
					/* done */
					free(buffer);
					fclose(f);
					if(err == ESP_OK)
						ESP_LOGI(TAG, "PSRAM file read OK");
					else
						ESP_LOGE(TAG, "PSRAM file failed");
				}
				else
					ESP_LOGI(TAG, "PSRAM file open error");
			}
		}
		else
			ESP_LOGI(TAG, "PSRAM file is empty");
	}
    else
		ESP_LOGI(TAG, "PSRAM file not found");
	
	/* configure FPGA from SPIFFS file */
	load_fpga(cfg_file);
	
    /* init ADC for Vbat readings */
    if(!adc_c3_init())
        ESP_LOGI(TAG, "ADC Initialized");
    else
        ESP_LOGW(TAG, "ADC Init Failed");
    
#if 1
	/* init WiFi & socket */
	if(!wifi_init())
	{
		ESP_LOGI(TAG, "WiFi Running");
		blink_period = 100;
	}
	else
		ESP_LOGE(TAG, "WiFi Init Failed");
#endif
	
	ESP_LOGI(TAG, "free heap: %"PRIu32"",esp_get_free_heap_size());
	
#if 1
	/* start up USB/serial command handler */
	if(!sercmd_init())
		ESP_LOGI(TAG, "Serial Command Running");
	else
		ESP_LOGE(TAG, "Serial Command Init Failed");
#endif
	
	/* wait here forever and blink */
    ESP_LOGI(TAG, "Looping...");
	gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
	uint8_t i = 0;
	while(1)
	{
		gpio_set_level(LED_PIN, i&1);
		
		if((i&15)==0)
		{
			//ESP_LOGI(TAG, "free heap: %d",esp_get_free_heap_size());
			//ESP_LOGI(TAG, "RSSI: %d", wifi_get_rssi());
			//ESP_LOGI(TAG, "Vbat = %d mV", 2*adc_c3_get());
		}
		
		i++;
		
		vTaskDelay(blink_period / portTICK_PERIOD_MS);
	}
}
