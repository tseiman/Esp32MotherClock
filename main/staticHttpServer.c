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
#include <sys/param.h>
#include <string.h>

#include <unistd.h>
#include <sys/fcntl.h>

#include <esp_https_server.h>
#include "esp_vfs.h"
#include "esp_spiffs.h"

#include "config.h"


#define HTTPS_SERVER_STACK_SIZE 32767; //16384
#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)


#define SCRATCH_BUFSIZE (10240)

static char scratch[SCRATCH_BUFSIZE];

static const char *TAG = "Esp32MotherClock.https";
static char *certBuffer = NULL;
static char *keyBuffer = NULL;

#define REGISTER_STATIC_RESOURCE(NAME,URI) \
static const httpd_uri_t NAME = { \
    .uri       = URI, \
    .method    = HTTP_GET, \
    .handler   = common_get_handler, \
    .user_ctx   = NULL, \
    .is_websocket = false \
}; \
httpd_register_uri_handler(server, &NAME);


#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath) {
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    } else if (CHECK_FILE_EXTENSION(filepath, ".jpg")) {
        type = "image/jpeg";
    }
    return httpd_resp_set_type(req, type);
}

/* Send HTTP response with the contents of the requested file */
static esp_err_t common_get_handler(httpd_req_t *req) {
    char filepath[FILE_PATH_MAX];

    strcpy(filepath,"/www");

    ESP_LOGI(TAG, "GET %s", req->uri);

    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    set_content_type_from_file(req, filepath);


    for(int i=0;filepath[i]!='\0';i++) {        // let's not take care about URL parameter here
        if(filepath[i]== '?') {
            filepath[i] = '\0';
            break;
        }

        if(filepath[i]== '.' && filepath[i+1]== '.') {              // let's not deal with funny relative pathes pointing us anywhere
            httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Invalid relative path");
            return ESP_FAIL;
        }
    }



    strlcat(filepath, ".gz", sizeof(filepath));

    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Connection", "keep-alive"); \


//    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd,  &scratch, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(TAG, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, (const char *)&scratch, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(TAG, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(TAG, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}


httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = common_get_handler,
        .user_ctx   = NULL, 
        .is_websocket = false 
};




httpd_handle_t start_static_webserver(void) {
    httpd_handle_t server = NULL;





    // Start the httpd server
    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.stack_size = HTTPS_SERVER_STACK_SIZE; 
    conf.httpd.lru_purge_enable = true;
    conf.httpd.uri_match_fn = httpd_uri_match_wildcard;
//    conf.httpd.max_open_sockets = CONFIG_MCLK_MAX_CLIENTS * 2;
    char *cert    = confToStringByKey("certificate");
    char *certkey = confToStringByKey("certificate-key");
    int cert_len = 0;
    int certkey_len = 0;
    if(cert != NULL && certkey != NULL) {
        cert_len = strnlen(cert,20000) +1;
        certkey_len = strnlen(certkey,20000) +1;
    }


    if((cert_len > 1) && (certkey_len > 1)) {
        ESP_LOGI(TAG, "Loading certificate from config");

        if((certBuffer = (char*)calloc(cert_len, sizeof(char))) == NULL) {                                                              
            ESP_LOGE(TAG, "Can't allocate buffer for certificate");
            return NULL;
        } 
        if((keyBuffer = (char*)calloc(certkey_len, sizeof(char))) == NULL) {                                                              
            ESP_LOGE(TAG, "Can't allocate buffer for private certificate key");
            free(certBuffer);
            certBuffer = NULL;
            return NULL;
        } 

        replaceEscapes(certBuffer, cert, cert_len);
        replaceEscapes(keyBuffer, certkey, certkey_len);


        conf.cacert_pem = (uint8_t *)certBuffer;
        conf.cacert_len = cert_len;
        conf.prvtkey_pem = (uint8_t *)keyBuffer;
        conf.prvtkey_len = certkey_len;

    } else {
        ESP_LOGI(TAG, "Loading default certificate");
        extern const unsigned char cacert_pem_start[] asm("_binary_cacert_pem_start");
        extern const unsigned char cacert_pem_end[]   asm("_binary_cacert_pem_end");
        conf.cacert_pem = cacert_pem_start;
        conf.cacert_len = cacert_pem_end - cacert_pem_start;

        extern const unsigned char prvtkey_pem_start[] asm("_binary_prvtkey_pem_start");
        extern const unsigned char prvtkey_pem_end[]   asm("_binary_prvtkey_pem_end");
        conf.prvtkey_pem = prvtkey_pem_start;
        conf.prvtkey_len = prvtkey_pem_end - prvtkey_pem_start;
    }


    esp_err_t ret = httpd_ssl_start(&server, &conf);
    if (ESP_OK != ret) {
        ESP_LOGI(TAG, "Error starting server!");
        free(certBuffer);
        free(keyBuffer);
        certBuffer = NULL;
        keyBuffer = NULL;
        return NULL;
    }

    // Set URI handlers
    ESP_LOGI(TAG, "Registering URI handlers");


 
    return server;
}


void registerStaticResources(httpd_handle_t server) {
       httpd_register_uri_handler(server, &common_get_uri);
}

void stop_static_webserver(httpd_handle_t server) {
    // Stop the httpd server
    if(certBuffer != NULL) free(certBuffer);
    if(keyBuffer != NULL) free(keyBuffer);
    httpd_ssl_stop(server);
}
