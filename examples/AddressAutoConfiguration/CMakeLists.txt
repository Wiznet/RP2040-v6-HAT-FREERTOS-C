set(TARGET_NAME w6x00_AAC)

add_executable(${TARGET_NAME}
        ${TARGET_NAME}.c
        AddressAutoConfig.c
        )

target_link_libraries(${TARGET_NAME} PRIVATE
        pico_stdlib
        hardware_spi
        hardware_dma
        FREERTOS_FILES
        ETHERNET_FILES
        IOLIBRARY_FILES
        LOOPBACK_FILES
        DHCP4_FILES
        DHCP6_FILES
        TIMER_FILES
        )

pico_enable_stdio_usb(${TARGET_NAME} 1)
pico_enable_stdio_uart(${TARGET_NAME} 0)

pico_add_extra_outputs(${TARGET_NAME})
