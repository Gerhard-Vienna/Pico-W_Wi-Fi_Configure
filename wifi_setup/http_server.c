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


#include "lwip/apps/httpd.h"
#include "http_server.h"
#include "pico/cyw43_arch.h"
#include "access_point.h"

/*
 * This file contains the code for SSI and CGI handling.
 *
 * Server Side Includes (SSI):
 * The tags (enclosed in "<!--" and "-->") embedded in web pages are
 * replaced by the server with dynamic text before the document is
 * delivered to the client. See ssi_handler().
 *
 * The Common Gateway Interface (CGI) allows a web server to delegate the
 * execution of a request.
 * This is exploited here by redirecting a call for a non-existent page to a
 * routine. See cgi_handler()
 */
const char * __not_in_flash("httpd") ssi_tags[] = {
    "SSID",    // 0
    "PASSWD",  // 1
    "B0",      // 2
    "B1",      // 3
    "B2",      // 4
    "B3",      // 5
    "B4",      // 6
    "B5",      // 7
    "B6",      // 8
    "B7",      // 9
    "B8",      // 10
    "B9",      // 11
    "B10",     // 12
    "B11",     // 13
};

#define HIGHLIGHT "STYLE=\"background-color: #72A4D2;\""
static uint8_t  lan[3][4];
bool _need_ip;
bool _need_gw;

static bool ip_err = false;
static bool mask_err = false;
static bool gw_err = false;

/*
 * ssi_init()
 *
 * Check ssi-tags for length and inizialize the ssi handler
 */

void ssi_init()
{
    size_t i;
    for (i = 0; i < LWIP_ARRAYSIZE(ssi_tags); i++) {
        LWIP_ASSERT("tag too long for LWIP_HTTPD_MAX_TAG_NAME_LEN",
                    strlen(ssi_tags[i]) <= LWIP_HTTPD_MAX_TAG_NAME_LEN);
    }

    http_set_ssi_handler(ssi_handler, ssi_tags, LWIP_ARRAYSIZE(ssi_tags));
}

/*
 * ssi_handler()
 *
 * SSI is triggered by the file extension ".shtml"
 */

u16_t __time_critical_func(ssi_handler)(int iIndex, char *pcInsert, int iInsertLen)
{
    // SSID and password may contain quotation marks which must be
    // converted to "&quote;" for the web site.
    // So we make the buffer twice the maximum size.
    // (and hope a full-length password doesn't contain
    // more than nine quotation marks)
    // static char webStr[PASSWD_MAX_LEN * 2];
    char webStr[PASSWD_MAX_LEN * 2];

    size_t printed = 0;
    switch (iIndex) {
        case 0: /* "SSID" */
            if(*(_c->ssid) == '\0'){
                printed = snprintf(pcInsert, iInsertLen, "%s", HIGHLIGHT);
                break;
            }
            encode_value(_c->ssid, webStr);
            printed = snprintf(pcInsert, iInsertLen, "value=\"%s\"", webStr);
            break;
        case 1: /* "password" */
        {
            encode_value(_c->passwd, webStr);
            printed = snprintf(pcInsert, iInsertLen, "value=\"%s\"", webStr);
        }
        break;

        case 2: /* "static ip address a */
        case 3: /* "static ip address b */
        case 4: /* "static ip address c */
        case 5: /* "static ip address d */
            if(_c->ip.addr == IPADDR_NONE){
                if(_need_ip)
                    printed = snprintf(pcInsert, iInsertLen, "%s", HIGHLIGHT);
            }
            else if(ip_err){
                printed = snprintf(pcInsert, iInsertLen, "value=\"%d\" %s",
                ip4_addr_get_byte(&(_c->ip), iIndex - 2), HIGHLIGHT);
            }
            else{
                printed = snprintf(pcInsert, iInsertLen, "value=\"%d\"",
                ip4_addr_get_byte(&(_c->ip), iIndex - 2));
            }
        break;

        case 6: /* "net mask address a */
        case 7: /* "net mask address b */
        case 8: /* "net mask address c */
        case 9: /* "net mask address d */
            if(_c->mask.addr == IPADDR_NONE){
                if(_need_ip)
                    printed = snprintf(pcInsert, iInsertLen, "%s", HIGHLIGHT);
            }
            else if(mask_err){
                printed = snprintf(pcInsert, iInsertLen, "value=\"%d\" %s",
                                   ip4_addr_get_byte(&(_c->mask), iIndex - 6), HIGHLIGHT);
            }
            else{
                printed = snprintf(pcInsert, iInsertLen, "value=\"%d\"",
                                   ip4_addr_get_byte(&(_c->mask), iIndex - 6));
            }
            break;

        case 10: /* "def gateway address a */
        case 11: /* "def gateway address b */
        case 12: /* "def gateway address c */
        case 13: /* "def gateway address d */
            if(_c->gw.addr == IPADDR_NONE){
                if(_need_gw)
                    printed = snprintf(pcInsert, iInsertLen, "%s", HIGHLIGHT);
            }
            else if(gw_err){
                printed = snprintf(pcInsert, iInsertLen, "value=\"%d\" %s",
                                   ip4_addr_get_byte(&(_c->gw), iIndex - 10), HIGHLIGHT);
            }
            else{
                printed = snprintf(pcInsert, iInsertLen, "value=\"%d\"",
                                   ip4_addr_get_byte(&(_c->gw), iIndex - 10));
            }
            break;
    }
    LWIP_ASSERT("sane length", printed <= 0xFFFF);
    return (u16_t)printed;
}

/*
 * encode_value()
 *
 * SSID and password may contain quotation marks which must be
 * converted to "&quote;" for the web site.
 */

void encode_value(char *src, char *dest)
{
    while(*src){
        if(*src == '"'){
            *dest++ = '&';
            *dest++ = 'q';
            *dest++ = 'u';
            *dest++ = 'o';
            *dest++ = 't';
            *dest++ = ';';
            src++;
        }
        else{
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}

/* Html request for "/setup.cgi" will start cgi_handler_setup */
static const tCGI cgi_handlers[] = {
    {"/setup.cgi", cgi_handler},
};

/*
 * cgi_init()
 *
 * initialize the CGI handler
 */

void
cgi_init(void)
{
    http_set_cgi_handlers(cgi_handlers, 1);
}

/*
 * cgi_handler()
 *
 * This cgi handler triggered by a request for "/setup.cgi"
 */

const char *
cgi_handler(int iIndex, int iNumParams, char *pcParam[], char *pcValue[])
{
    memset(lan, 0, sizeof(lan));
    ip_err   = false;
    mask_err = false;
    gw_err   = false;

    for (int i = 0; i < iNumParams; i++){
        if(strcmp(pcParam[i], "ssid") == 0){
            url_decode(pcValue[i], _c->ssid);
        }
        else if(strcmp(pcParam[i], "passwd") == 0){
            url_decode(pcValue[i], _c->passwd);
        }
        else if(pcParam[i][0] == 'B'){
            uint8_t index = atoi(&(pcParam[i][1]));
            int val = atoi(pcValue[i]);
            switch(index){
                case 0:
                case 1:
                case 2:
                case 3: // IP address
                    if(pcValue[i][0] == '\0' && _need_ip)
                        ip_err = true;
                    else if(val < 0 || val > 255)
                        ip_err = true;
                    else
                        lan[index/4][index%4] = val;
                    break;

                case 4:
                case 5:
                case 6:
                case 7: // net mask
                    if(pcValue[i][0] == '\0' && _need_ip)
                        mask_err = true;
                    else if(val < 0 || val > 255)
                        mask_err = true;
                    else
                        lan[index/4][index%4] = val;
                    break;

                case 8:
                case 9:
                case 10:
                case 11: // default gateway
                    if(pcValue[i][0] == '\0' && _need_gw)
                        gw_err = true;
                    else if(val < 0 || val > 255)
                        gw_err = true;
                    else
                        lan[index/4][index%4] = val;
                    break;

            }
        }
    }
    IP4_ADDR(&(_c->ip),   lan[0][0], lan[0][1], lan[0][2], lan[0][3]);
    if(!_c->ip.addr)
        _c->ip.addr = IPADDR_NONE;

    IP4_ADDR(&(_c->mask), lan[1][0], lan[1][1], lan[1][2], lan[1][3]);
    if(!_c->mask.addr)
        _c->mask.addr = IPADDR_NONE;

    IP4_ADDR(&(_c->gw),   lan[2][0], lan[2][1], lan[2][2], lan[2][3]);
    if(!_c->gw.addr)
        _c->gw.addr = IPADDR_NONE;


    DEBUG_printf("IP %s\n", ip4addr_ntoa(&(_c->ip)));
    DEBUG_printf("NM %s\n", ip4addr_ntoa(&(_c->mask)));
    DEBUG_printf("GW %s\n", ip4addr_ntoa(&(_c->gw)));
    if(!(ip_err || mask_err || gw_err))
        DEBUG_printf("Configure OK\n");
    else
        DEBUG_printf("Configure ERROR\n");

    if(!(ip_err || mask_err || gw_err)){
        _c->magic = MAGIC;
        isConfigured = true;
        return "/done.html";
    }
    else{
        return "/index.shtml";
    }
}

/*
 * url_decode()
 *
 * Chars, not allowed in an url, but contained in a get request
 * are encoded.
 * " " as "+" and all others as hex encoded ascii code.
 */

void url_decode(char *src, char *dest)
{
    while(*src){
        if(*src == '+'){
            *dest = ' ';
            src++;
            dest++;
        }
        else if(*src == '%'){
            char a[3];

            a[0] = *++src;
            a[1] = *++src;
            a[2] = '\0';
            *dest++ = strtol(a, NULL, 16);
            src++;
        }
        else{
            *dest++ = *src++;
        }
    }
    *dest = '\0';
}


