idf_component_register(SRCS "uc-rv32ima.c"
			"port-esp.c"
			"jit-call.S"
		       LDFRAGMENTS "link.lf"
                       INCLUDE_DIRS "."
                       PRIV_REQUIRES "sdmmc" "esp_driver_sdmmc" "vfs" "fatfs")

if(CONFIG_EMU_SOFTWARE_CACHE)
	target_sources(${COMPONENT_LIB} PRIVATE "cache.c")
endif()

# the interpreter loop is the hot path
set_source_files_properties(uc-rv32ima.c PROPERTIES COMPILE_OPTIONS "-O3")
