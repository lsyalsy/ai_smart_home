set(COMPONENT_ADD_INCLUDEDIRS "include")

idf_component_register(SRCS "sqlite3.c" "esp32.c" "shox96_0_2.c"
                       INCLUDE_DIRS "include"
                       PRIV_INCLUDE_DIRS "private_include"
                       REQUIRES mbedtls
                       PRIV_REQUIRES console spiffs spi_flash esp_hw_support)

target_compile_options(${COMPONENT_LIB} PRIVATE -std=gnu99 -g3 -fno-stack-protector -ffunction-sections -fdata-sections -fstrict-volatile-bitfields -mlongcalls -nostdlib -Wpointer-arith -Wno-error=unused-value -Wno-error=unused-label -Wno-error=unused-function -Wno-error=unused-but-set-variable -Wno-unused-variable -Wno-error=deprecated-declarations -Wno-char-subscripts -Wno-maybe-uninitialized -Wno-unused-parameter -Wno-sign-compare -Wno-old-style-declaration -Wno-missing-field-initializers -Wno-type-limits -Wno-cast-function-type -Wno-extra -Wno-unused-function -Wno-unused-label -Wno-unused-value -Wno-unused-but-set-variable -MMD -c -DF_CPU=240000000L -DESP32 -DCORE_DEBUG_LEVEL=0 -DNDEBUG)