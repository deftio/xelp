cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(xelp_pico_cli C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(xelp_pico_cli
    main.c
    xelp.c
)

target_include_directories(xelp_pico_cli PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(xelp_pico_cli
    pico_stdlib
    hardware_gpio
    hardware_adc
    hardware_pwm
)

# USB serial (not UART)
pico_enable_stdio_usb(xelp_pico_cli 1)
pico_enable_stdio_uart(xelp_pico_cli 0)

# Pico W LED requires the CYW43 driver
if (PICO_BOARD STREQUAL "pico_w" OR PICO_BOARD STREQUAL "pico2_w")
    target_link_libraries(xelp_pico_cli pico_cyw43_arch_none)
endif()

pico_add_extra_outputs(xelp_pico_cli)
