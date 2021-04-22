/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * Here we handle WebsocketIO
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/task.h"

#include <esp_http_server.h>

#include "cJSON.h"

#include "commandHandler.h"


#include "websocket.h"

static const char *TAG = "Esp32MotherClock.ws";

#define STACK_SIZE_JANITOR_TASK 4096 // 2048




TaskHandle_t janitorTaskHandle = NULL;


static httpd_handle_t serverRef = NULL;


/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg) {
   
    async_resp_arg_t resp_arg = arg;
    ESP_LOGI(TAG, "send async message: %s", resp_arg->msg);
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;


    sessionContext_t sess = httpd_sess_get_ctx(hd, fd);

    if(sess == NULL) {
        ESP_LOGE(TAG, "No session for this async send (%d) ! ",fd);
        return;
    }

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)resp_arg->msg;
    ws_pkt.len = resp_arg->len;
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);

    free(resp_arg->msg);

    if(resp_arg->lastMessage) {
        free(resp_arg); 
    }
}



 esp_err_t trigger_async_send(async_resp_arg_t arg) {
    ESP_LOGI(TAG, "trigger_async_send");
    return httpd_queue_work(arg->hd, ws_async_send, arg);
}



#define JANNITOR_TASK_ASSERT_ALLOC(comparison, msg) \
    if (comparison) {                               \
        ESP_LOGE(TAG, msg);                         \
        break;                                      \
    }

#define JANNITOR_TASK_ASSERT_ALLOC_JSON(comparison) JANNITOR_TASK_ASSERT_ALLOC(comparison, "Error creating JSON")

void sessionJanitorTask( void* pvParameters ) {
  for (;;) {
    if(serverRef != NULL) {
        size_t clients = CONFIG_MCLK_MAX_CLIENTS * 10;
        int    client_fds[CONFIG_MCLK_MAX_CLIENTS * 10];

        
        if (httpd_get_client_list(serverRef, &clients, client_fds) == ESP_OK) {
            for (size_t i=0; i < clients; ++i) {
                int sock = client_fds[i];
                if (httpd_ws_get_fd_info(serverRef, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
                    ESP_LOGI(TAG, "Active client (fd=%d) -> sending PING message", sock);
                    
                    async_resp_arg_t resp_arg = malloc(sizeof(struct async_resp_arg));
                    resp_arg->hd = serverRef;
                    resp_arg->fd = sock;

                    cJSON *pingMsgObj = cJSON_CreateObject();
                    JANNITOR_TASK_ASSERT_ALLOC_JSON(pingMsgObj == NULL);

                    cJSON *cmd = cJSON_CreateString("PING");
                    JANNITOR_TASK_ASSERT_ALLOC_JSON(cmd == NULL);

                    cJSON_AddItemToObject(pingMsgObj, "cmd", cmd);

                    char *jsonStrPtr = cJSON_Print(pingMsgObj);
                    resp_arg->msg = calloc(strlen(jsonStrPtr) + 1, sizeof(char));
                    strcpy(resp_arg->msg, jsonStrPtr);
                    resp_arg->lastMessage = true;
                    resp_arg->len = strlen(resp_arg->msg); 

                    cJSON_Delete(pingMsgObj);

                    JANNITOR_TASK_ASSERT_ALLOC(httpd_queue_work(resp_arg->hd, ws_async_send, resp_arg) != ESP_OK, "httpd_queue_work failed!");

                    
                }

            }
        } else {
            ESP_LOGE(TAG, "Error httpd_get_client_list returned ESP_ERR_INVALID_ARG");
        }

    }

    ESP_LOGI(TAG, "Janitor task");


    vTaskDelay(5000 / portTICK_PERIOD_MS);
  } // END FOR 
  vTaskDelete( NULL );
}



/*
 * Is called to clean up a session context
 */

void freeSessionContext(void *ctx) {
    ESP_LOGI(TAG, "Freeing up session context");
    free(ctx);
}


/*
 * This handler is called in case a incomming WS mesage has to be handled
 */

static esp_err_t incomming_ws_message_handler(httpd_req_t *req) {

    uint8_t *inputBuffer = NULL;

    sessionContext_t sessContext = httpd_sess_get_ctx(req->handle, httpd_req_to_sockfd(req));

    httpd_sess_update_lru_counter(req->handle, httpd_req_to_sockfd(req));

    
    if (req->method == HTTP_GET) {                        // this is executed in case the first connection to websocket
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");

        if(sessContext == NULL) {
            ESP_LOGI(TAG, "New session context");
            sessContext = (sessionContext_t)malloc(sizeof(sessionContext)); 
         //   sessContext->notSeenCount = 0;
            sessContext->authenticated = false;
            httpd_sess_set_ctx(req->handle, httpd_req_to_sockfd(req), sessContext, freeSessionContext);
        } else {
            ESP_LOGW(TAG, "Very odd - for a new conenction there should be no connection");
        }

        return ESP_OK;
    }

                                                            // in case a new connection it should have been handled before
    if(sessContext == NULL) {                               // in case an existing connection the session context can't be NULL
        ESP_LOGE(TAG, "Not a new connection - Session context should be set ?!");
        return ESP_ERR_NOT_FOUND;
    }


    httpd_ws_frame_t websocketPacket;              // allocate a structure to extract the request
   
    memset(&websocketPacket, 0, sizeof(httpd_ws_frame_t));
    websocketPacket.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &websocketPacket, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", websocketPacket.len);




    if (websocketPacket.len) {

        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        inputBuffer = calloc(1, websocketPacket.len + 1);
        if (inputBuffer == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        websocketPacket.payload = inputBuffer;
        /* Set max_len = websocketPacket.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &websocketPacket, websocketPacket.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(inputBuffer);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", websocketPacket.payload);
    }
    ESP_LOGI(TAG, "Packet type: %d", websocketPacket.type);


    if(websocketPacket.type != HTTPD_WS_TYPE_TEXT ) {
        ESP_LOGE(TAG, "Invalid Packet Type");
        ret = ESP_FAIL;
        goto END_INCOMMING_WS_ERROR_MESSAGE_HANDLER;
    }

    async_resp_arg_t resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);

    ret = interpreteCmd((char*) websocketPacket.payload, websocketPacket.len + 1, resp_arg, sessContext, &trigger_async_send);

    if (ret != ESP_OK) {
            ESP_LOGE(TAG, "incomming WS Command could be interpreted");
            ret = ESP_OK;
            goto END_INCOMMING_WS_ERROR_MESSAGE_HANDLER;
    }

    goto END_INCOMMING_WS_MESSAGE_HANDLER;



END_INCOMMING_WS_ERROR_MESSAGE_HANDLER: ;

    cJSON *errMsgObj = cJSON_CreateObject();
    if (errMsgObj == NULL) {
        ESP_LOGE(TAG, "could allocate memory for ERROR msg obj");
    }
    cJSON *cmd = cJSON_CreateString("ERROR");
    if (cmd == NULL) {
        ESP_LOGE(TAG, "could allocate memory for ERROR msgcmd");
    }

    cJSON_AddItemToObject(errMsgObj, "cmd", cmd);



    websocketPacket.type = HTTPD_WS_TYPE_TEXT;
    websocketPacket.payload = (uint8_t *) cJSON_Print(errMsgObj);
    websocketPacket.len =  (size_t) strlen( (char *) websocketPacket.payload);
    httpd_ws_send_frame(req, &websocketPacket);

    cJSON_Delete(errMsgObj);

END_INCOMMING_WS_MESSAGE_HANDLER:
    free(inputBuffer);
    return ret;
}

static const httpd_uri_t wss = {
        .uri        = "/ws",
        .method     = HTTP_GET,
        .handler    = incomming_ws_message_handler,
        .user_ctx   = NULL,
        .is_websocket = true
};


void start_wss(httpd_handle_t server) {
    ESP_LOGI(TAG, "Registering WebSocket handler");
    serverRef = server;
    httpd_register_uri_handler(server, &wss);
    xTaskCreate( sessionJanitorTask, "WSSsessionJanitorTask", STACK_SIZE_JANITOR_TASK, NULL, tskIDLE_PRIORITY + 1, &janitorTaskHandle );
}

void stop_wss(httpd_handle_t server) {
    ESP_LOGI(TAG, "Stop WebSocket handler");
}

