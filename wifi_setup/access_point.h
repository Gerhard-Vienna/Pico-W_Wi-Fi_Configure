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

#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include "lwip/ip4_addr.h"
#include "flash_program.h"

/*
 * define's you may change to suit your needs...
 */
#define AP_SSID     "picow_config"
//#define AP_PASSWD "my secret" // define, if you want a password
#define MAGIC       0xCAFE      // used to check if the flash contains valid data
#define SETUP_GPIO  22          // pull this GPIO to GND to force the steup mode
#define SETUP_DELAY 3           // duration for wich SETUP_GPIO must be held low

#define DEBUG   // Uncomment for debug output

#ifdef DEBUG
#define DEBUG_printf(...) printf(__VA_ARGS__)
#else
#define DEBUG_printf(...)
#endif

/*
 * define's you should not modify
 */
#define TCP_PORT 67

#define SSID_MAX_LEN        32
#define PASSWD_MAX_LEN      63


typedef struct _config {
    uint16_t magic;
    char     ssid[SSID_MAX_LEN + 1];
    char     passwd[PASSWD_MAX_LEN + 1];
    ip4_addr_t ip;
    ip4_addr_t mask;
    ip4_addr_t gw;
} config;

extern config *_c;
extern bool _need_ip;
extern bool _need_gw;
extern bool isConfigured;

bool forceSetup();
void run_access_point(config *config, bool req_static_ip, bool req_def_gateway);

#endif // ACCESS_POINT_H
