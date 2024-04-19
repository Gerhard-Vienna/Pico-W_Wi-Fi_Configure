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
 *
 * It is based on code from the "pico-sdk".
 * See the copyright notice below.
 */

/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"

#include "tcp_test_server.h"

// #define DEBUG_printf(...) printf(__VA_ARGS__)
#define DEBUG_printf(...)


//lwIP error codes
const char * err_names[] = {
    "ERR_OK",       // 0
    "ERR_MEM",      // -1
    "ERR_BUF",      // -2
    "ERR_TIMEOUT",  // -3
    "ERR_RTE",      // -4
    "ERR_INPROGRESS",// -5
    "ERR_VAL",      // -6
    "ERR_WOULDBLOCK",// -7
    "ERR_USE",      // -8
    "ERR_ALREADY",  // -9
    "ERR_ISCONN",   // -10
    "ERR_CONN",     // -11
    "ERR_IF",       // -12
    "ERR_ABRT",     // -13
    "ERR_RST",      // -14
    "ERR_CLSD",     // -15
    "ERR_ARG"       // -16
};

void(* clear_config) (void);

const char *lwip_err_str(int err)
{
    if(err > ERR_OK || err < ERR_ARG)
        return("unkown error code");
    else
        return(err_names[-err]);
}

static TCP_SERVER_T* tcp_server_init(void) {
    TCP_SERVER_T *state = calloc(1, sizeof(TCP_SERVER_T));
    if (!state) {
        DEBUG_printf("failed to allocate state\n");
        return NULL;
    }
    return state;
}

static err_t tcp_server_close(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    err_t err = ERR_OK;
    if (state->client_pcb != NULL) {
        tcp_arg(state->client_pcb, NULL);
        tcp_sent(state->client_pcb, NULL);
        tcp_recv(state->client_pcb, NULL);
        tcp_err(state->client_pcb, NULL);
        err = tcp_close(state->client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(state->client_pcb);
            err = ERR_ABRT;
        }
        state->client_pcb = NULL;
    }
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
    return err;
}

static err_t tcp_server_exit(void *arg, int status) {
   TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (status == 0) {
        DEBUG_printf("test success\n");
    } else {
        DEBUG_printf("test failed %d\n", status);
    }
    state->complete = true;
    return tcp_server_close(arg);
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    state->sent_len += len;

    if (state->sent_len >= BUF_SIZE) {
        // We should get the data back from the client
        state->recv_len = 0;
    }

    return ERR_OK;
}

err_t tcp_server_send_data(void *arg, struct tcp_pcb *tpcb)
{
   TCP_SERVER_T *state = (TCP_SERVER_T*)arg;

    state->sent_len = 0;
    DEBUG_printf("Writing %d bytes to client: \"%s\"\n", strlen(state->buffer_sent), state->buffer_sent);
    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    err_t err = tcp_write(tpcb, state->buffer_sent, strlen(state->buffer_sent), TCP_WRITE_FLAG_COPY);
    if (err != ERR_OK) {
        DEBUG_printf("Failed to write data %d\n", err);
        return tcp_server_exit(arg, -1);
    }
    return ERR_OK;
}

err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (!p) {
        if(err == ERR_OK)
            printf("Client disconnected\n");
        else
            DEBUG_printf("tcp_server_recv p = NULL: %s\n", lwip_err_str(err));
        err = tcp_close(tpcb);
        if(err != ERR_OK)
            DEBUG_printf("tcp_close: %s (%d)\n", lwip_err_str(err), err);
        return ERR_OK;
    }

   // TODO handle this gracefully...
    if(err != ERR_OK) {
        DEBUG_printf("tcp_server_recv: %s (%d)\n", lwip_err_str(err), err);
        return tcp_server_exit(arg, -1);
    }

    // this method is callback from lwIP, so cyw43_arch_lwip_begin is not required, however you
    // can use this method to cause an assertion in debug mode, if this method is called when
    // cyw43_arch_lwip_begin IS needed
    cyw43_arch_lwip_check();
    if (p->tot_len > 0) {
        // Receive the buffer
        const uint16_t buffer_left = BUF_SIZE - state->recv_len;
        state->recv_len += pbuf_copy_partial(p, state->buffer_recv + state->recv_len,
                                             p->tot_len > buffer_left ? buffer_left : p->tot_len, 0);
        tcp_recved(tpcb, p->tot_len);
    }
    pbuf_free(p);

    // Have we have received the whole buffer
    if (strstr(state->buffer_recv, ".")) {
        state->buffer_recv[state->recv_len] = '\0';

        // Send  buffer
        memcpy(state->buffer_sent, state->buffer_recv, state->recv_len + 1);
        memset(state->buffer_recv, '\0', BUF_SIZE);
        state->recv_len = 0;

        return tcp_server_send_data(arg, state->client_pcb);
    }
    else if (strcmp(state->buffer_recv, "erase!") == 0) {
        clear_config();

        // Send  buffer
        strcpy(state->buffer_sent, "Erasing flash requested!");
        memset(state->buffer_recv, '\0', BUF_SIZE);
        state->recv_len = 0;

        return tcp_server_send_data(arg, state->client_pcb);
    }

    return ERR_OK;
}

static void tcp_server_err(void *arg, err_t err) {
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_server_exit(arg, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK) {
        DEBUG_printf("Failure in accept: %s\n", lwip_err_str(err));
        tcp_server_exit(arg, err);
        return ERR_VAL;
    }
    if (client_pcb == NULL) {
        DEBUG_printf("Failure in accept: client_pcb == NULL\n");
        tcp_server_exit(arg, err);
        return ERR_VAL;
    }

    state->client_pcb = client_pcb;
    tcp_arg(client_pcb, state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_err(client_pcb, tcp_server_err);

    printf("Client connected\n");
    return ERR_OK;
}

static bool tcp_server_open(void *arg) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    printf("Starting tcp server at %s on port %u\n",
           ip4addr_ntoa(netif_ip4_addr(netif_list)), TEST_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, NULL, TEST_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n", TEST_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    return true;
}

void run_tcp_server( void (*f)(void) ) {
    clear_config = f;

    TCP_SERVER_T *state = tcp_server_init();
    if (!state) {
        return;
    }
    if (!tcp_server_open(state)) {
        tcp_server_exit(state, -1);
        return;
    }
    while(!state->complete) {
       // This sleep is just an example of some (blocking) work you might be doing.
        sleep_ms(1000);
    }
    free(state);
}

