idf_component_register(SRCS "main.c"
							"ice.c"
							"spiffs.c"
							"wifi.c"
                            "adc_c3.c"
                    INCLUDE_DIRS "")
# Create a SPIFFS image from the contents of the 'spiffs_image' directory
#spiffs_create_partition_image(storage ../spiffs FLASH_IN_PROJECT)
