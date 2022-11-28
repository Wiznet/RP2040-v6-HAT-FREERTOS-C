# Ethernet
add_library(ETHERNET_FILES STATIC)

target_sources(ETHERNET_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet/socket.c
        ${WIZNET_DIR}/Ethernet/wizchip_conf.c
        )

if(${WIZNET_CHIP} STREQUAL W6100)
target_include_directories(ETHERNET_FILES INTERFACE
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Ethernet/W6100
        )

target_link_libraries(ETHERNET_FILES PUBLIC
        W6100_FILES
        )
endif()

add_library(W6100_FILES STATIC)

target_sources(W6100_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet/W6100/w6100.c
        )

target_include_directories(W6100_FILES INTERFACE
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Ethernet/W6100
        )

target_link_libraries(W6100_FILES PRIVATE
        ETHERNET_FILES
        )

# Loopback
add_library(LOOPBACK_FILES STATIC)

target_sources(LOOPBACK_FILES PUBLIC
        ${WIZNET_DIR}/Application/loopback/loopback.c
        )

target_include_directories(LOOPBACK_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Application/loopback
        )

# DHCP4
add_library(DHCP4_FILES STATIC)

target_sources(DHCP4_FILES PUBLIC
        ${WIZNET_DIR}/Internet/DHCP4/dhcpv4.c
        )

target_include_directories(DHCP4_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Ethernet/W6100
        ${WIZNET_DIR}/Internet/DHCP4
        )

# DHCP6
add_library(DHCP6_FILES STATIC)

target_sources(DHCP6_FILES PUBLIC
        ${WIZNET_DIR}/Internet/DHCP6/dhcpv6.c
        )

target_include_directories(DHCP6_FILES PUBLIC
        ${WIZNET_DIR}/Ethernet
        ${WIZNET_DIR}/Ethernet/W6100
        ${WIZNET_DIR}/Internet/DHCP6
        )

target_link_libraries(ETHERNET_FILES PUBLIC
        W6100_FILES
        )

# FreeRTOS
add_library(FREERTOS_FILES STATIC)

target_sources(FREERTOS_FILES PUBLIC
        ${FREERTOS_DIR}/event_groups.c
        ${FREERTOS_DIR}/list.c
        ${FREERTOS_DIR}/queue.c
        ${FREERTOS_DIR}/stream_buffer.c
        ${FREERTOS_DIR}/tasks.c
        ${FREERTOS_DIR}/timers.c
        ${FREERTOS_DIR}/portable/GCC/ARM_CM0/port.c  		
        ${FREERTOS_DIR}/portable/MemMang/heap_4.c
        )

target_include_directories(FREERTOS_FILES PUBLIC
        ${PORT_DIR}/FreeRTOS-Kernel/inc
        ${FREERTOS_DIR}/include
        ${FREERTOS_DIR}/portable/GCC/ARM_CM0
        )