/**
 * Copyright (c) 2021 WIZnet Co.,Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * ----------------------------------------------------------------------------------------------------
 * Includes
 * ----------------------------------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>

#include "port_common.h"

#include "wizchip_conf.h"
#include "w6x00_spi.h"

#include "loopback.h"
#include "AddressAutoConfig.h"
#include "dhcpv4.h"

#include "timer.h"

/**
 * ----------------------------------------------------------------------------------------------------
 * Macros
 * ----------------------------------------------------------------------------------------------------
 */
/* Task */
#define ADDRS_AUTO_CONFIG_TASK_STACK_SIZE 2048
#define ADDRS_AUTO_CONFIG_TASK_PRIORITY 8

#define TCP_SERVER_TASK_STACK_SIZE 512
#define TCP_SERVER_TASK_PRIORITY 10

#define TCP_CLIENT_TASK_STACK_SIZE 512
#define TCP_CLIENT_TASK_PRIORITY 10

#define UDP_TASK_STACK_SIZE 512
#define UDP_TASK_PRIORITY 10

#define TCP_SERVER6_TASK_STACK_SIZE 512
#define TCP_SERVER6_TASK_PRIORITY 10

#define TCP_CLIENT6_TASK_STACK_SIZE 512
#define TCP_CLIENT6_TASK_PRIORITY 10

#define UDP6_TASK_STACK_SIZE 512
#define UDP6_TASK_PRIORITY 10

#define DNS_TASK_STACK_SIZE 512
#define DNS_TASK_PRIORITY 10

/* Clock */
#define PLL_SYS_KHZ (133 * 1000)

/* Buffer */
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)

/* Socket */
#define SOCKET_TCP_SERVER 0
#define SOCKET_TCP_CLIENT 1
#define SOCKET_UDP 2
#define SOCKET_TCP_SERVER6 3
#define SOCKET_TCP_CLIENT6 4
#define SOCKET_UDP6 5
#define SOCKET_TCP_SERVER_DUAL 6
#define SOCKET_DHCP 7

/* Port */
#define PORT_TCP_SERVER 5000
#define PORT_TCP_CLIENT 5001
#define PORT_TCP_CLIENT_DEST    5002
#define PORT_UDP 5003

#define PORT_TCP_SERVER6 5004
#define PORT_TCP_CLIENT6 5005
#define PORT_TCP_CLIENT6_DEST 5006
#define PORT_UDP6 5007

#define PORT_TCP_SERVER_DUAL 5008

#define IPV4
#define IPV6

#ifdef IPV4
#define TCP_SERVER
#define TCP_CLIENT
#define UDP
#define DHCP_RETRY_COUNT 5
#define DHCP4
#endif

#ifdef IPV6
#define TCP_SERVER6
#define TCP_CLIENT6
#define UDP6
#define ADDRS_AUTO_CONFIG
#endif

#if defined IPV4 && defined IPV6
#define TCP_SERVER_DUAL
#endif

/**
 * ----------------------------------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------------------------------------
 */
/* Network */
static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x56}, // MAC address
        .ip = {192, 168, 11, 2},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 11, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
        .lla = {0xfe, 0x80, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x02, 0x08, 0xdc, 0xff,
                0xfe, 0x57, 0x57, 0x25},             // Link Local Address
        .gua = {0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00},             // Global Unicast Address
        .sn6 = {0xff, 0xff, 0xff, 0xff,
                0xff, 0xff, 0xff, 0xff,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00},             // IPv6 Prefix
        .gw6 = {0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00},             // Gateway IPv6 Address
        .dns6 = {0x20, 0x01, 0x48, 0x60,
                0x48, 0x60, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x88, 0x88},             // DNS6 server
        .ipmode = 0
};

uint8_t tcp_client_destip[] = {
    192, 168, 50, 103
};

uint8_t tcp_client_destip6[] = {
    0x20, 0x01, 0x02, 0xb8,
    0x00, 0x10, 0xff, 0xff,
    0x71, 0x48, 0xcb, 0x27,
    0x36, 0xb9, 0x99, 0x2e
};

uint16_t tcp_client_destport = PORT_TCP_CLIENT_DEST;

uint16_t tcp_client_destport6 = PORT_TCP_CLIENT6_DEST;

static uint8_t g_ethernet_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
}; // common buffer

/* Loopback */
static uint8_t g_tcp_server_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_tcp_client_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_udp_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_tcp_server6_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_tcp_client6_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_udp6_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_tcp_server_dual_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};

/* DHCP */
static uint8_t g_dhcp_get_ip_flag = 0;

static uint8_t g_aac_get_ip6_flag = 0;

/* Timer  */
static volatile uint32_t g_msec_cnt = 0;

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Task */
void addrs_auto_config_task(void *argument);
void tcp_server_task(void *argument);
void tcp_client_task(void *argument);
void udp_task(void *argument);
void tcp_server6_task(void *argument);
void tcp_client6_task(void *argument);
void udp6_task(void *argument);

/* Clock */
static void set_clock_khz(void);

/* DHCP */
static void wizchip_dhcp4_init(void);
static void wizchip_dhcp4_assign(void);
static void wizchip_dhcp4_conflict(void);

/* Timer  */
static void repeating_timer_callback(void);

/**
 * ----------------------------------------------------------------------------------------------------
 * Main
 * ----------------------------------------------------------------------------------------------------
 */
int main()
{
    /* Initialize */
    int retval = 0;

    set_clock_khz();

    stdio_init_all();

    sleep_ms(1000 * 3);

    printf("==========================================================\n");
    printf("Compiled @ %s, %s\n", __DATE__, __TIME__);
    printf("==========================================================\n");

    wizchip_spi_initialize();
    wizchip_cris_initialize();

    wizchip_reset();
    wizchip_initialize();
    wizchip_check();

    wizchip_1ms_timer_initialize(repeating_timer_callback);

    #ifdef DHCP4
    g_net_info.ipmode = NETINFO_DHCP_V4;
    #else
    g_net_info.ipmode = NETINFO_STATIC_V4;
    #endif

    #ifdef IPV6
    #ifdef ADDRS_AUTO_CONFIG
    g_net_info.ipmode |= NETINFO_SLAAC_V6;
    #else
    g_net_info.ipmode |= NETINFO_STATIC_V6;
    #endif
    #endif

    xTaskCreate(addrs_auto_config_task, "ADDRS_AUTO_CONFIG_TASK", ADDRS_AUTO_CONFIG_TASK_STACK_SIZE, NULL, ADDRS_AUTO_CONFIG_TASK_PRIORITY, NULL);

    xTaskCreate(tcp_server_task, "TCP_SERVER_Task", TCP_SERVER_TASK_STACK_SIZE, NULL, TCP_SERVER_TASK_PRIORITY, NULL);
    xTaskCreate(tcp_client_task, "TCP_CLIENT_Task", TCP_CLIENT_TASK_STACK_SIZE, NULL, TCP_CLIENT_TASK_PRIORITY, NULL);
    xTaskCreate(udp_task, "UDP_Task", UDP_TASK_STACK_SIZE, NULL, UDP_TASK_PRIORITY, NULL);

    xTaskCreate(tcp_server6_task, "TCP_SERVER6_Task", TCP_SERVER6_TASK_STACK_SIZE, NULL, TCP_SERVER6_TASK_PRIORITY, NULL);
    xTaskCreate(tcp_client6_task, "TCP_CLIENT6_Task", TCP_CLIENT6_TASK_STACK_SIZE, NULL, TCP_CLIENT6_TASK_PRIORITY, NULL);
    xTaskCreate(udp6_task, "UDP6_Task", UDP6_TASK_STACK_SIZE, NULL, UDP6_TASK_PRIORITY, NULL);
    
    //xTaskCreate(dns_task, "DNS_Task", DNS_TASK_STACK_SIZE, NULL, DNS_TASK_PRIORITY, NULL);

    printf("Start vTaskStartScheduler()\n");

    vTaskStartScheduler();

    while (1)
    {
        ;
    }
}

/**
 * ----------------------------------------------------------------------------------------------------
 * Functions
 * ----------------------------------------------------------------------------------------------------
 */
/* Task */
void addrs_auto_config_task(void *argument)
{
    int retval = 0;
    uint8_t dhcp_retry = 0;

    printf("Start addrs_auto_config_task()\n");

    printf("network_initialize()\n");
    network_initialize(g_net_info);

    #ifdef ADDRS_AUTO_CONFIG
    if(g_net_info.ipmode & NETINFO_SLAAC_V6)
    {
        printf("AddressAutoConfig_Init()\n");
        if(1 != AddressAutoConfig_Init(&g_net_info))
        {
            printf("Address Auto Config failed\n");
            g_aac_get_ip6_flag = 0;
        }
        else
        {
            g_aac_get_ip6_flag = 1;
        }
    }
    #endif

    printf("Address Auto Config done\n");

    #ifdef DHCP4
    if (g_net_info.ipmode & NETINFO_DHCP_V4) // DHCP
    {
        wizchip_dhcp4_init();
    }
    #else
    g_dhcp_get_ip_flag = 1;
    #endif

    while (1)
    {
        #ifdef DHCP4
        /* Assigned IP through DHCP */
        if (g_net_info.ipmode & NETINFO_DHCP_V4)
        {
            retval = DHCPv4_run();
            //printf("%d retval = 0x%x(%d)\n", __LINE__, retval, retval);

            if (retval == DHCP_IP_LEASED)
            {
                if (g_dhcp_get_ip_flag == 0)
                {
                    printf(" DHCP success\n");

                    g_dhcp_get_ip_flag = 1;
                }
            }
            else if (retval == DHCP_FAILED)
            {
                g_dhcp_get_ip_flag = 0;
                dhcp_retry++;

                if (dhcp_retry <= DHCP_RETRY_COUNT)
                {
                    printf(" DHCP timeout occurred and retry %d\n", dhcp_retry);
                }
            }

            if (dhcp_retry > DHCP_RETRY_COUNT)
            {
                printf(" DHCP failed\n");

                DHCPv4_stop();

                while (1)
                    ;
            }

            if(g_dhcp_get_ip_flag == 0)
            {
                vTaskDelay(10);
            }
        }
        #else
        vTaskDelay(10);
        #endif
    }
}

void tcp_server_task(void *argument)
{
    int retval = 0;

    printf("Start tcp_server_task()\n");

    while(g_dhcp_get_ip_flag != 1)
    {
        vTaskDelay(10);
    }

    while (1)
    {
        if ((retval = loopback_tcps(SOCKET_TCP_SERVER, g_tcp_server_buf, PORT_TCP_SERVER, AS_IPV4)) < 0)
        {
            printf(" loopback_tcps error : %d\n", retval);
        }
        vTaskDelay(10);
    }
}

void tcp_client_task(void *argument)
{
    int retval = 0;

    printf("Start tcp_client_task()\n");

    while(g_dhcp_get_ip_flag != 1)
    {
        vTaskDelay(10);
    }

    while (1)
    {
        if ((retval = loopback_tcpc(SOCKET_TCP_CLIENT, g_tcp_client_buf, tcp_client_destip, tcp_client_destport, AS_IPV4)) < 0)
        {
            printf(" loopback_tcpc error : %d\n", retval);
        }
        vTaskDelay(10);
    }
}

void udp_task(void *argument)
{
    int retval = 0;

    printf("Start udp_task()\n");

    while(g_dhcp_get_ip_flag != 1)
    {
        vTaskDelay(10);
    }

    while (1)
    {
        if ((retval = loopback_udps(SOCKET_UDP, g_udp_buf, PORT_UDP, AS_IPV4)) < 0)
        {
            printf(" loopback_udps error : %d\n", retval);
        }
        vTaskDelay(10);
    }
}

void tcp_server6_task(void *argument)
{
    int retval = 0;

    printf("Start tcp_server6_task()\n");

    while(g_dhcp_get_ip_flag != 1)
    {
        vTaskDelay(10);
    }

    while (1)
    {
        if ((retval = loopback_tcps(SOCKET_TCP_SERVER6, g_tcp_server6_buf, PORT_TCP_SERVER6, AS_IPV6)) < 0)
        {
            printf(" loopback_tcps IPv6 error : %d\n", retval);
        }
        vTaskDelay(10);
    }
}

void tcp_client6_task(void *argument)
{
    int retval = 0;

    printf("Start tcp_client6_task()\n");

    while(g_dhcp_get_ip_flag != 1)
    {
        vTaskDelay(10);
    }

    while (1)
    {
        if ((retval = loopback_tcpc(SOCKET_TCP_CLIENT6, g_tcp_client6_buf, tcp_client_destip6, tcp_client_destport6, AS_IPV6)) < 0)
        {
            printf(" loopback_tcpc IPv6 error : %d\n", retval);
        }
        vTaskDelay(10);
    }
}

void udp6_task(void *argument)
{
    int retval = 0;

    printf("Start udp6_task()\n");

    while(g_dhcp_get_ip_flag != 1)
    {
        vTaskDelay(10);
    }

    while (1)
    {
        if ((retval = loopback_udps(SOCKET_UDP6, g_udp6_buf, PORT_UDP6, AS_IPV6)) < 0)
        {
            printf(" loopback_udps IPv6 error : %d\n", retval);
        }
        vTaskDelay(10);
    }
}

void dns_task(void *argument)
{
    printf("Start dns_task()\n");

    while (1)
    {
        
        vTaskDelay(10);
    }
}

/* Clock */
static void set_clock_khz(void)
{
    // set a system clock frequency in khz
    set_sys_clock_khz(PLL_SYS_KHZ, true);

    // configure the specified clock
    clock_configure(
        clk_peri,
        0,                                                // No glitchless mux
        CLOCKS_CLK_PERI_CTRL_AUXSRC_VALUE_CLKSRC_PLL_SYS, // System PLL on AUX mux
        PLL_SYS_KHZ * 1000,                               // Input frequency
        PLL_SYS_KHZ * 1000                                // Output (must be same as no divider)
    );
}

/* DHCP */
static void wizchip_dhcp4_init(void)
{
    close(SOCKET_DHCP);
    
    printf(" DHCP client running\n");

    DHCPv4_init(SOCKET_DHCP, g_ethernet_buf);

    reg_dhcpv4_cbfunc(wizchip_dhcp4_assign, wizchip_dhcp4_assign, wizchip_dhcp4_conflict);
}

static void wizchip_dhcp4_assign(void)
{
    getIPfromDHCPv4(g_net_info.ip);
    getGWfromDHCPv4(g_net_info.gw);
    getSNfromDHCPv4(g_net_info.sn);
    getDNSfromDHCPv4(g_net_info.dns);

    /* Network initialize */
    network_initialize(g_net_info); // apply from DHCP

    print_network_information(g_net_info);
    printf(" DHCP leased time : %ld seconds\n", getDHCPv4Leasetime());
}

static void wizchip_dhcp4_conflict(void)
{
    printf(" Conflict IP from DHCP\n");

    // halt or reset or any...
    while (1)
        ; // this example is halt.
}

/* Timer */
static void repeating_timer_callback(void)
{
    g_msec_cnt++;

    if (g_msec_cnt >= 1000 - 1)
    {
        g_msec_cnt = 0;

        DHCPv4_time_handler();
#if 0
        DNS_time_handler();
#endif
    }
}