# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# 由于部分硬件组件放在二级子目录中，需要显式加入 EXTRA_COMPONENT_DIRS
set(EXTRA_COMPONENT_DIRS
    components/actuator/led
    components/actuator/buzzer
    components/actuator/motor
    components/actuator/relay
    components/input/ec11
    components/sensors/bh1750
    components/sensors/dht22
    components/sensors/hcsr501
    components/sensors/mq2
    components/st7735
    components/ui
    components/ble
    components/audio/inmp441
    components/voice)

project(ai_smart_home)
