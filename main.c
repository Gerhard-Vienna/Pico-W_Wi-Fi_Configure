/**
 * This file is part of "Wi-Fi Configure.
 *
 * This software eliminates the need to know the network name, password and,
 * if required, IP address, network mask and default gateway at compile time.
 * These can be set directly on the Pico-W and also changed afterwards.
 *
 * Copyright (c) 2024 Gerhard Schiller gerhard.schiller@pm.me
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwipopts.h"

#include "access_point.h"
#include "tcp_test_server.h"

void print_config(config *c) {
    if(c->magic != MAGIC) {
        printf("No configuration found.\n");
        return;
    }

    printf("Stored configuration data:\n");
    printf("\tMagic:        %04X\n",  c->magic);
    printf("\tSSID:        \"%s\"\n", c->ssid);
    printf("\tPassword:    \"%s\"\n", c->passwd);
    printf("\tIP:           %s\n",    ip4addr_ntoa(&(c->ip)));
    printf("\tNetmask:      %s\n",    ip4addr_ntoa(&(c->mask)));
    printf("\tDef. Gateway: %s\n",    ip4addr_ntoa(&(c->gw)));
}

void clear_flash(void)
{
    config config;

    printf("Client has requested the erasure of the configuration\n");
    flash_erase_page(WIFI_CONFIG_PAGE, 1);
    flash_read((uint8_t *)&config, sizeof(config), WIFI_CONFIG_PAGE);
    print_config(&config);

}

void main(void) {
    config config;

    stdio_init_all();
    if (cyw43_arch_init()) {
        printf("failed to initialise\n");
        return;
    }

    printf("Starting Wifi Configure\n");
    show_stats();

/* Configuration code starts here */
    flash_read((uint8_t *)&config, sizeof(config), WIFI_CONFIG_PAGE);

    if(config.magic != MAGIC || forceSetup()){
        printf("\nPico is in config mode!\n");

// Modify according to your requirements.
        // Static IP and default gateway are optional
        run_access_point(&config, false, false);

        // Static IP is required, default gateway is optional
//      run_access_point(&config, true, false);
        // Static IP and default gateway are required
//      run_access_point(&config, true, true);

        // store the configuration in flash memory
        flash_write_page((uint8_t *)&config, sizeof(config), WIFI_CONFIG_PAGE);
    }
    print_config(&config);
/* Configuration code ends here */


/* Code for your device starts here */
    printf("\nPico is in run mode!\n");
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    // cyw43_arch_enable_sta_mode();
    cyw43_arch_enable_sta_mode();
    if(config.ip.addr != IPADDR_NONE){
#if LWIP_DHCP == 1
        dhcp_release_and_stop(netif_default);
#endif
        netif_set_addr(netif_default, &(config.ip), &(config.mask), &(config.gw));
        netif_set_up(netif_default);
#if LWIP_DHCP == 1
        dhcp_inform(netif_default);
#endif
        printf("Using static IP: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_default)));
    }
    else{
        printf("Using DHCP: ");
    }

    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(config.ssid, config.passwd, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return;
    }
    else {
        printf("connected.\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    }

    // Just to show you what can be done...
    run_tcp_server(clear_flash);

    // NOT_REACHED
    return;
}
