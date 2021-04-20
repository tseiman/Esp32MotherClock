

#pragma once

/* #include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>

#include <esp_http_server.h>

*/


struct sessionContext {
    int notSeenCount;   
    bool authenticated;
} sessionContext;
typedef struct sessionContext* sessionContext_t;

void start_wss(httpd_handle_t server);
void stop_wss(httpd_handle_t server);
