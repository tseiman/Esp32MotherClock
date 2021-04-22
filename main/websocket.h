/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * Websocket defintions and prototypes
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */

#pragma once



struct sessionContext {
//    int notSeenCount;   
    bool authenticated;
} sessionContext;
typedef struct sessionContext* sessionContext_t;

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
    char *msg;
    int len;
    bool lastMessage;
} async_resp_arg;
typedef struct async_resp_arg* async_resp_arg_t;

typedef esp_err_t (*asyncCallback_t)(async_resp_arg_t arg);


void start_wss(httpd_handle_t server);
void stop_wss(httpd_handle_t server);
