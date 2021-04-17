#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>
#include "freertos/task.h"

#include <esp_http_server.h>

#include "cJSON.h"


static const char *TAG = "Esp32MotherClock.ws";

#define STACK_SIZE_JANITOR_TASK 4096 // 2048

#define MSG_PING "PING"





// static const max_clients = CONFIG_MCLK_MAX_CLIENTS;

/*
 * Structure holding server handle
 * and internal socket fd in order
 * to use out of request send
 */
struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
    char *msg;
};


struct sessionContext {
    int notSeenCount;   
    bool authenticated;
} sessionContext;
typedef struct sessionContext* sessionContext_t;


TaskHandle_t janitorTaskHandle = NULL;


static httpd_handle_t serverRef = NULL;


/*
 * async send function, which we put into the httpd work queue
 */
static void ws_async_send(void *arg) {
   
    struct async_resp_arg *resp_arg = arg;
    ESP_LOGI(TAG, "send async message: %s", resp_arg->msg);
    httpd_handle_t hd = resp_arg->hd;
    int fd = resp_arg->fd;


    sessionContext_t sess = httpd_sess_get_ctx(hd, fd);

    if(sess == NULL) {
        ESP_LOGE(TAG, "No session for this async send (%d) ! ",fd);
        return;
    }


    if(sess->notSeenCount >= 4) {
        ESP_LOGW(TAG, "Client with socketfd=%d did no answer on communicaiton %d times, Closing socket ! ",fd,sess->notSeenCount);
        return;
    }

    if(sess->notSeenCount > 0) {
        ESP_LOGW(TAG, "Client with socketfd=%d did no answer on communicaiton %d times ! ",fd,sess->notSeenCount);
        return;
    }

    ++sess->notSeenCount;

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)resp_arg->msg;
    ws_pkt.len = strlen(resp_arg->msg);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    httpd_ws_send_frame_async(hd, fd, &ws_pkt);
    free(resp_arg->msg);
    free(resp_arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req) {
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, resp_arg);
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
        size_t clients = CONFIG_MCLK_MAX_CLIENTS;
        int    client_fds[CONFIG_MCLK_MAX_CLIENTS];

        
        if (httpd_get_client_list(serverRef, &clients, client_fds) == ESP_OK) {
            for (size_t i=0; i < clients; ++i) {
                int sock = client_fds[i];
                if (httpd_ws_get_fd_info(serverRef, sock) == HTTPD_WS_CLIENT_WEBSOCKET) {
                    ESP_LOGI(TAG, "Active client (fd=%d) -> sending PING message", sock);
                    
                    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
                    resp_arg->hd = serverRef;
                    resp_arg->fd = sock;
/*                    resp_arg->msg = malloc(strlen(MSG_PING) +1);
                    strcpy(resp_arg->msg, MSG_PING); */



                    cJSON *pingMsgObj = cJSON_CreateObject();
                    JANNITOR_TASK_ASSERT_ALLOC_JSON(pingMsgObj == NULL);

                    cJSON *cmd = cJSON_CreateString("PING");
                    JANNITOR_TASK_ASSERT_ALLOC_JSON(cmd == NULL);

                    cJSON_AddItemToObject(pingMsgObj, "cmd", cmd);

                    resp_arg->msg = cJSON_Print(pingMsgObj);

                    cJSON_Delete(pingMsgObj);

                    JANNITOR_TASK_ASSERT_ALLOC(httpd_queue_work(resp_arg->hd, ws_async_send, resp_arg) != ESP_OK, "httpd_queue_work failed!");

                    
                }

            }
        }

    }

    ESP_LOGI(TAG, "Janitor task");


    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }  
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

#define IF_JSON_CMD(CMD_TO_COMPARE)  if(ws_pkt.type == HTTPD_WS_TYPE_TEXT && cJSON_IsString(cmd) && (cmd->valuestring != NULL) && strcmp(cmd->valuestring,  CMD_TO_COMPARE) == 0 ) 


#define COMMAND_ASSERT_ALLOC_JSON(comparison,obj)   \
    if (comparison) {                               \
        ESP_LOGE(TAG, "Error creating JSON");       \
        free(buf);                                  \
         cJSON_Delete(cmd);                         \
        return ESP_FAIL;                            \
    }


#define  GOTO_END_INCOMMING_WS_MESSAGE_HANDLER(RET_VALUE) \
     ret = RET_VALUE; \
     goto END_INCOMMING_WS_MESSAGE_HANDLER;


static esp_err_t incomming_ws_message_handler(httpd_req_t *req) {

    uint8_t *buf = NULL;
  //  sessionContext_t sessContext = NULL;


    sessionContext_t sessContext = httpd_sess_get_ctx(req->handle, httpd_req_to_sockfd(req));

    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");

        if(sessContext == NULL) {
            ESP_LOGI(TAG, "New session context");
            sessContext = (sessionContext_t)malloc(sizeof(sessionContext)); 
            sessContext->notSeenCount = 0;
            sessContext->authenticated = false;
            httpd_sess_set_ctx(req->handle, httpd_req_to_sockfd(req), sessContext, freeSessionContext);
        } else {
            ESP_LOGW(TAG, "Very odd - for a new conenction there should be no connection");
        }

      //  sessContext->notSeenCount = 0;

        return ESP_OK;
    }



    if(sessContext == NULL) {
        ESP_LOGE(TAG, "Not a new connection - Session context should be set ?!");
        return ESP_ERR_NOT_FOUND;
    }

    sessContext->notSeenCount = 0;


    httpd_ws_frame_t ws_pkt;
   
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    ESP_LOGI(TAG, "frame len is %d", ws_pkt.len);
    if (ws_pkt.len) {

        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            free(buf);
            return ret;
        }
        ESP_LOGI(TAG, "Got packet with message: %s", ws_pkt.payload);
    }
    ESP_LOGI(TAG, "Packet type: %d", ws_pkt.type);
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && strcmp((char*)ws_pkt.payload,"Trigger async") == 0) {
        free(buf);
        return trigger_async_send(req->handle, req);
    }


    cJSON *json = cJSON_ParseWithLength((char *) ws_pkt.payload,ws_pkt.len + 1);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { 
            ESP_LOGE(TAG, "Error before: %s", error_ptr);
        }
        ESP_LOGE(TAG, "could not parse incomming message as JSON");
        free(buf);
        return ESP_FAIL;
    }

    cJSON *cmd = cJSON_GetObjectItemCaseSensitive(json, "cmd");


  //  if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && cJSON_IsString(cmd) && (cmd->valuestring != NULL) && strcmp(cmd->valuestring,  MSG_PONG) == 0 ) {
    IF_JSON_CMD("PONG") {
        ESP_LOGI(TAG, "Got PONG Response");
        GOTO_END_INCOMMING_WS_MESSAGE_HANDLER(ESP_OK);
    }

     IF_JSON_CMD("GETSTATUS") {
        ESP_LOGI(TAG, "Got Status Request");

        cJSON *statusMsgObj = cJSON_CreateObject();
        COMMAND_ASSERT_ALLOC_JSON(statusMsgObj == NULL,statusMsgObj);
        cJSON *cmd = cJSON_CreateString("STATUS");
        COMMAND_ASSERT_ALLOC_JSON(cmd == NULL,statusMsgObj);
        cJSON_AddItemToObject(statusMsgObj, "cmd", cmd);

        cJSON *auth = cJSON_CreateBool(sessContext->authenticated);
//        COMMAND_ASSERT_ALLOC_JSON(auth == NULL,statusMsgObj);
        cJSON_AddItemToObject(statusMsgObj, "auth", auth);

        ws_pkt.payload =  (uint8_t *)cJSON_Print(statusMsgObj);
        if(ws_pkt.payload == NULL) {
            ESP_LOGE(TAG, "Can't allocate space for Status JSON");
            GOTO_END_INCOMMING_WS_MESSAGE_HANDLER(ESP_OK);
        }
        ws_pkt.len = strlen((char*) ws_pkt.payload);
        ws_pkt.type = HTTPD_WS_TYPE_TEXT;

        ret = httpd_ws_send_frame(req, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_send_frame failed with %d", ret);
        }
       
        free(ws_pkt.payload);
        GOTO_END_INCOMMING_WS_MESSAGE_HANDLER(ESP_OK);

    }


END_INCOMMING_WS_MESSAGE_HANDLER:

    cJSON_Delete(cmd);
    free(buf);
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
 //   configASSERT( janitorTaskHandle );

}

void stop_wss(httpd_handle_t server) {
    ESP_LOGI(TAG, "Stop WebSocket handler");
}

