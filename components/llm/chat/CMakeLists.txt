idf_component_register(SRCS "chat.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_http_client esp_tls esp_wifi cjson)
