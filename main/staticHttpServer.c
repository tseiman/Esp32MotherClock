/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * The file contains the static HTTPS code
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */

#include "freertos/FreeRTOS.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
// #include <nvs_flash.h>
#include <sys/param.h>
// #include "esp_netif.h"
// #include "esp_eth.h"

#include <esp_https_server.h>
// #include "websocket.h"

static const char *TAG = "Esp32MotherClock.https";



#define STATIC_RESOURCE_HANDLER(DATA_NAME, HANDLER_NAME, CONTENT_TYPE) \
static esp_err_t HANDLER_NAME(httpd_req_t *req){ \
    extern const unsigned char HANDLER_NAME##_gz_start[] asm("_binary_"DATA_NAME"_gz_start"); \
    extern const unsigned char HANDLER_NAME##_gz_end[]   asm("_binary_"DATA_NAME"_gz_end"); \
    size_t gz_len = HANDLER_NAME##_gz_end - HANDLER_NAME##_gz_start; \
    httpd_resp_set_type(req, CONTENT_TYPE); \
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip"); \
    return httpd_resp_send(req, (const char *)HANDLER_NAME##_gz_start, gz_len); \
}

//     httpd_resp_set_hdr(req, "Connection", "keep-alive"); 

#define REGISTER_STATIC_RESOURCE(NAME,URI,HANDLER_NAME) \
static const httpd_uri_t NAME = { \
    .uri       = URI, \
    .method    = HTTP_GET, \
    .handler   = HANDLER_NAME, \
    .user_ctx   = NULL, \
    .is_websocket = false \
};


STATIC_RESOURCE_HANDLER("index_html",index_html_handler,"text/html");
REGISTER_STATIC_RESOURCE(indexroot, "/",index_html_handler);
REGISTER_STATIC_RESOURCE(indexhtml, "/index.html",index_html_handler);




httpd_handle_t start_static_webserver(void) {
    httpd_handle_t server = NULL;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.stack_size = 16384;


    extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
    extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
    conf.cacert_pem = cacert_pem_start;
    conf.cacert_len = cacert_pem_end - cacert_pem_start;

    extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
    extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
    conf.prvtkey_pem = prvtkey_pem_start;
    conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;

    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");

    httpd_register_uri_handler(server, &indexroot);
    httpd_register_uri_handler(server, &indexhtml);
// start_wss(server);
    return server;
}

void stop_static_webserver(httpd_handle_t server) {
    // Stop the httpd server
    httpd_ssl_stop(server);
}
