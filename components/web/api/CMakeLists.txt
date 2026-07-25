idf_component_register(SRCS "api_routes.c"
                    INCLUDE_DIRS "include"
                    EMBED_FILES "../static/index.html"
                    REQUIRES esp_http_server led buzzer motor relay
                             dht22 bh1750 hcsr501 bathroom_pir mq2 ui chat)
