/**
 * This file is part of "Wi-Fi Configure.
 *
 * This software makes it unnecessary to know the network name, password
 * and - if required - IP address, network mask and default gateway when
 * compiling. These can be set directly on the Pico-W and also modified
 * afterwards.
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
        run_access_point(&config, false, false);
//          run_access_point(&config, true, false);
//        run_access_point(&config, true, true);

        // store the configuration in flash memory
        flash_write_page((uint8_t *)&config, sizeof(config), WIFI_CONFIG_PAGE);
    }
    print_config(&config);
/* Configuration code ends here */


/* Code for your device starts here */
    printf("\nPico is in run mode!\n");

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(config.ssid, config.passwd, CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return;
    }
    else {
        printf("connected.\n");
    }
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);

    if(config.ip.addr != IPADDR_NONE){
        printf("Using static IP: ");
        netif_set_addr(netif_default, &(config.ip), &(config.mask), &(config.gw));
        netif_set_up(netif_default);
    }
    else{
        printf("Using DHCP: ");
    }
    printf("%s\n",
           ip4addr_ntoa(netif_ip4_addr(netif_default)));

    // Just an example what you can do...
    run_tcp_server(clear_flash);

    // NOT_REACHED
    return;
}
