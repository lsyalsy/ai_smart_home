idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES st7735 ui driver
                             led buzzer motor relay ec11
                             hcsr501 bathroom_pir dht22 bh1750 mq2
                             ble
                             state notify
                             inmp441 voice
                             http_server)
