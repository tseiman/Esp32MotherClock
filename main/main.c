/* Simple HTTP + SSL Server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
// #include "freertos/task.h"


#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include "esp_netif.h"
#include "esp_eth.h"

#include "sdkconfig.h"

#include "driver/gpio.h"

#include <esp_https_server.h>

/* A simple example that demonstrates how to create GET and POST
 * handlers and start an HTTPS server.
*/

static const char *TAG = "example";



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
};


STATIC_RESOURCE_HANDLER("index_html",index_html_handler,"text/html");
REGISTER_STATIC_RESOURCE(indexroot, "/",index_html_handler);
REGISTER_STATIC_RESOURCE(indexhtml, "/index.html",index_html_handler);

/*
STATIC_RESOURCE_HANDLER("main_js", main_js_handler,"application/javascript");
REGISTER_STATIC_RESOURCE(mainjs, "/main.js", main_js_handler);


STATIC_RESOURCE_HANDLER("style_css",style_css_handler,"text/css");
REGISTER_STATIC_RESOURCE(stylecss, "/style.css", style_css_handler);


STATIC_RESOURCE_HANDLER("jquery_slim_min_js",jquery_slim_min_js_handler,"application/javascript");
REGISTER_STATIC_RESOURCE(jquery, "/jquery.slim.min.js", jquery_slim_min_js_handler);


STATIC_RESOURCE_HANDLER("favicon_ico",favicon_ico_handler,"image/x-icon");
REGISTER_STATIC_RESOURCE(favicon, "/favicon.ico", favicon_ico_handler);
*/




static httpd_handle_t start_webserver(void)
{
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
/*    httpd_register_uri_handler(server, &mainjs);
    httpd_register_uri_handler(server, &stylecss);
    httpd_register_uri_handler(server, &jquery);
    httpd_register_uri_handler(server, &favicon);
*/

    return server;
}

static void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_ssl_stop(server);
}

static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        stop_webserver(*server);
        *server = NULL;
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        *server = start_webserver();
    }
}


/** Event handler for Ethernet events */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    uint8_t mac_addr[6] = {0};
    /* we can get the ethernet driver handle from event data */
    esp_eth_handle_t eth_handle = *(esp_eth_handle_t *)event_data;

    switch (event_id) {
    case ETHERNET_EVENT_CONNECTED:
        esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
        ESP_LOGI(TAG, "Ethernet Link Up");
        ESP_LOGI(TAG, "Ethernet HW Addr %02x:%02x:%02x:%02x:%02x:%02x",
                 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
        break;
    case ETHERNET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Ethernet Link Down");
        break;
    case ETHERNET_EVENT_START:
        ESP_LOGI(TAG, "Ethernet Started");
        break;
    case ETHERNET_EVENT_STOP:
        ESP_LOGI(TAG, "Ethernet Stopped");
        break;
    default:
        break;
    }
}

/** Event handler for IP_EVENT_ETH_GOT_IP */
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
    const esp_netif_ip_info_t *ip_info = &event->ip_info;

    ESP_LOGI(TAG, "Ethernet Got IP Address");
    ESP_LOGI(TAG, "~~~~~~~~~~~");
    ESP_LOGI(TAG, "ETHIP:" IPSTR, IP2STR(&ip_info->ip));
    ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
    ESP_LOGI(TAG, "ETHGW:" IPSTR, IP2STR(&ip_info->gw));
    ESP_LOGI(TAG, "~~~~~~~~~~~");
}

#define	PIN_PHY_POWER	12					

void app_main(void)
{
    static httpd_handle_t server = NULL;

    ESP_LOGI(TAG, "============");
    ESP_LOGI(TAG, " Esp32 Simple HTTPS");
    ESP_LOGI(TAG, " (c) 2021, TS");
    ESP_LOGI(TAG, "============");



    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());


    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);

    // Set default handlers to process TCP/IP stuffs
    ESP_ERROR_CHECK(esp_eth_set_default_handlers(eth_netif));


    /* Register event handlers to start server when Ethernet is connected,
     * and stop server when disconnection happens.
     */

    // Register user defined event handers
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &got_ip_event_handler, NULL));

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, &connect_handler, &server));
    ESP_ERROR_CHECK(esp_event_handler_register(ETH_EVENT, ETHERNET_EVENT_DISCONNECTED, &disconnect_handler, &server));



    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();
    phy_config.phy_addr = CONFIG_EXAMPLE_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
    gpio_pad_select_gpio(PIN_PHY_POWER);
    gpio_set_direction(PIN_PHY_POWER,GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_PHY_POWER, 1);
    vTaskDelay(pdMS_TO_TICKS(10));										
#if CONFIG_EXAMPLE_USE_INTERNAL_ETHERNET
    mac_config.smi_mdc_gpio_num = CONFIG_EXAMPLE_ETH_MDC_GPIO;
    mac_config.smi_mdio_gpio_num = CONFIG_EXAMPLE_ETH_MDIO_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
#if CONFIG_EXAMPLE_ETH_PHY_IP101
    esp_eth_phy_t *phy = esp_eth_phy_new_ip101(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_RTL8201
    esp_eth_phy_t *phy = esp_eth_phy_new_rtl8201(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_LAN8720
    esp_eth_phy_t *phy = esp_eth_phy_new_lan8720(&phy_config);
#elif CONFIG_EXAMPLE_ETH_PHY_DP83848
    esp_eth_phy_t *phy = esp_eth_phy_new_dp83848(&phy_config);
#endif
#elif CONFIG_EXAMPLE_USE_DM9051
    gpio_install_isr_service(0);
    spi_device_handle_t spi_handle = NULL;
    spi_bus_config_t buscfg = {
        .miso_io_num = CONFIG_EXAMPLE_DM9051_MISO_GPIO,
        .mosi_io_num = CONFIG_EXAMPLE_DM9051_MOSI_GPIO,
        .sclk_io_num = CONFIG_EXAMPLE_DM9051_SCLK_GPIO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(CONFIG_EXAMPLE_DM9051_SPI_HOST, &buscfg, 1));
    spi_device_interface_config_t devcfg = {
        .command_bits = 1,
        .address_bits = 7,
        .mode = 0,
        .clock_speed_hz = CONFIG_EXAMPLE_DM9051_SPI_CLOCK_MHZ * 1000 * 1000,
        .spics_io_num = CONFIG_EXAMPLE_DM9051_CS_GPIO,
        .queue_size = 20
    };
    ESP_ERROR_CHECK(spi_bus_add_device(CONFIG_EXAMPLE_DM9051_SPI_HOST, &devcfg, &spi_handle));
    /* dm9051 ethernet driver is based on spi driver */
    eth_dm9051_config_t dm9051_config = ETH_DM9051_DEFAULT_CONFIG(spi_handle);
    dm9051_config.int_gpio_num = CONFIG_EXAMPLE_DM9051_INT_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_dm9051(&dm9051_config, &mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_dm9051(&phy_config);
#endif
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    /* attach Ethernet driver to TCP/IP stack */
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));
    /* start Ethernet driver state machine */
    ESP_ERROR_CHECK(esp_eth_start(eth_handle));


start_webserver();
//    ESP_ERROR_CHECK(example_connect());

}
