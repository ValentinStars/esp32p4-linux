idf_component_register(SRCS "uc-rv32ima.c"
			"port-esp.c"
			"cyberdeck.c"
			"jit-call.S"
		       LDFRAGMENTS "link.lf"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES "sdmmc" "esp_driver_sdmmc" "vfs" "fatfs" "usb" "usb_host_hid" "usb_host_msc" "esp_lcd" "esp_lcd_ek79007" "esp_hw_support")

if(CONFIG_EMU_SOFTWARE_CACHE)
	target_sources(${COMPONENT_LIB} PRIVATE "cache.c")
endif()

# the interpreter loop is the hot path
set_source_files_properties(uc-rv32ima.c PROPERTIES COMPILE_OPTIONS "-O3")
