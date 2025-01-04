/**
 * This file is part of "Wi-Fi Configure.
 *
 * This software eliminates the need to know the network name, password and,
 * if required, IP address, network mask and default gateway at compile time.
 * These can be set directly on the Pico-W and also changed afterwards.
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
#include "lwip/apps/httpd.h"

#include "access_point.h"
#include "http_server.h"
#include "dhcp_server.h"

config *_c;
bool isConfigured = false;

static void run_http_server();

/*
 * void run_access_point(config *config,
 *                          bool require_static_ip,
 *                          bool require_def_gateway)
 * config:          pointer to configuration data
 *                  at entry:   data found in the flash
 *                  at return:  user entries
 * req_static_ip:   user must provide an IP-adress and a netmask
 * req_def_gateway. user must provide the deafault gateways IP-address
 *
 * Creates an access point and starts a web server that provides a page for
 * configuring the access data and the LAN.
 * After entering the data on this page, the access point is terminated.
 */

void run_access_point(config *config, bool req_static_ip, bool req_def_gateway)
{
    _c = config;
    _need_ip = req_static_ip;
    _need_gw = req_def_gateway;

    if(_c->magic != MAGIC){
        memset(_c->ssid, '\0', SSID_MAX_LEN);
        memset(_c->passwd, '\0', PASSWD_MAX_LEN);
        _c->ip.addr = IPADDR_NONE;
        _c->mask.addr = IPADDR_NONE;
        _c->gw.addr = IPADDR_NONE;
     }

    const char *ap_name = AP_SSID;
#ifdef AP_PASSWD
    const char *password = AP_PASSWD;
#else
    const char *password = NULL;    // no password
#endif

/*
 * To test the configuration page, it is recommended to run the
 * HTTP server on your local network.
 * Define LOCAL_TEST and enter your WIFI name and password at
 * cyw43_arch_wifi_connect_timeout_ms(....).
*/
#ifndef LOCAL_TEST
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);

    ip4_addr_t ip, gw, mask;
    IP4_ADDR(&ip,   192, 168,   0, 1);
    IP4_ADDR(&mask, 255, 255, 255, 0);
    IP4_ADDR(&gw,   192, 168,   0, 1);
    netif_set_ipaddr(netif_default, &ip);
    netif_set_up(netif_default);

    // Start the dhcp server
    dhcp_server_t dhcp_server;
    dhcp_server_init(&dhcp_server, &gw, &mask);

    DEBUG_printf("Access point for configuration created\n");
    DEBUG_printf("SSID: \"%s\", PSK: \"%s\"\n", ap_name, password?password:"none");

#else
    cyw43_arch_enable_sta_mode();
    printf("Connecting to WiFi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms("MY_SSID", "MY_PASSWORD", CYW43_AUTH_WPA2_AES_PSK, 30000)) {
        printf("failed to connect.\n");
        return;
    }
    else {
        printf("connected.\n");
    }
    printf("IP-Address: %s Port: %d\n",
                 ip4addr_ntoa(netif_ip4_addr(netif_default)), HTTPD_SERVER_PORT);
#endif

    // and the http server
    run_http_server();

    while(!isConfigured) {
        static absolute_time_t led_time;
        static int led_on = true;

        // flash the led to show that we are in config mode
        if (absolute_time_diff_us(get_absolute_time(), led_time) < 0) {
            led_on = !led_on;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_on);
            led_time = make_timeout_time_ms(250);
        }
        sleep_ms(1);
    }
    // disable config modes
#ifndef LOCAL_TEST
    dhcp_server_deinit(&dhcp_server);
    cyw43_arch_disable_ap_mode();
#endif
    DEBUG_printf("Configuration done!\n");
}

/*
 * forceSetup()
 *
 * Checks whether the Config button is pressed during startup and
 * then for another 3 seconds.
 */

bool forceSetup(){
    gpio_init(SETUP_GPIO);
    gpio_set_dir(SETUP_GPIO, GPIO_IN);
    gpio_pull_up(SETUP_GPIO);
    if(!gpio_get(SETUP_GPIO)){
        static absolute_time_t reset_time;
        reset_time = make_timeout_time_ms(SETUP_DELAY * 1000);
        while (absolute_time_diff_us(get_absolute_time(), reset_time) > 0 && !gpio_get(SETUP_GPIO))
            sleep_ms(1);

        if(!gpio_get(SETUP_GPIO)){
            DEBUG_printf("Re-run setup requested\n");
            return true;
        }
    }
    return false;
}

/*
 * run_http_server()
 *
 * the web server that provides the page for configuring the access data
 * and the LAN
 */

static void run_http_server() {
    httpd_init();
    ssi_init();
    cgi_init();
    DEBUG_printf("HTTP-Server for setup initialized.\n");
    DEBUG_printf("IP-Address: %s Port: %d\n",
           ip4addr_ntoa(netif_ip4_addr(netif_default)), HTTPD_SERVER_PORT);
}

