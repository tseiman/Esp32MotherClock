

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include "freertos/task.h"

#include <esp_http_server.h>

#include "cJSON.h"

#include "commandHandler.h"

#include "websocket.h"



static const char *TAG = "Esp32MotherClock.wsCmd";


 
static esp_err_t buffer2JSON(char *inputBuffer, size_t inputBufferLen, cJSON **json) {
    (*json) = cJSON_ParseWithLength(inputBuffer,inputBufferLen);

    if (*json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) { 
            ESP_LOGE(TAG, "Error before: %s", error_ptr);
        }
        ESP_LOGE(TAG, "could not parse incomming message as JSON");
        return ESP_FAIL;
    }
    return ESP_OK;
}



#define IF_JSON_CMD(CMD_TO_COMPARE)  if(cJSON_IsString(cmd) && (cmd->valuestring != NULL) && strcmp(cmd->valuestring,  CMD_TO_COMPARE) == 0) 


#define COMMAND_ASSERT_ALLOC_JSON(comparison,obj)   \
    if (comparison) {                               \
        ESP_LOGE(TAG, "Error creating JSON");       \
        cJSON_Delete(cmd);                          \
        return ESP_FAIL;                            \
    }

#define CREATE_JSON_COMMAND(COMMAND,OBJECT_NAME) \
        cJSON *OBJECT_NAME = cJSON_CreateObject(); \
        COMMAND_ASSERT_ALLOC_JSON(OBJECT_NAME == NULL,OBJECT_NAME); \
        cJSON *cmd = cJSON_CreateString(COMMAND); \
        COMMAND_ASSERT_ALLOC_JSON(cmd == NULL,OBJECT_NAME); \
        cJSON_AddItemToObject(OBJECT_NAME, "cmd", cmd);

#define CREATE_JSON_BOOL_PARAMETER(PARAMETER_NAME,JSON_ROOT_OBJECT_NAME,OBJECT_NAME,VALUE) \
        cJSON *OBJECT_NAME = cJSON_CreateBool(VALUE); \
        cJSON_AddItemToObject(JSON_ROOT_OBJECT_NAME, PARAMETER_NAME, OBJECT_NAME);

/*
#define JSON_TO_DOWNSTREAM_BUFFER(JSON_ROOT_OBJECT_NAME,COMMAND) \
        downstreamPacket_t pkt = *downstreamPacket; \
        char *buff=cJSON_Print(JSON_ROOT_OBJECT_NAME); \
        if(buff == NULL) { \
            ESP_LOGE(TAG, "Can't allocate buffer space from JSON for message " COMMAND); \
            return ESP_ERR_NO_MEM; \
        } \
        pkt = malloc(sizeof(downstreamPkt)); \
        if(pkt == NULL) { \
            ESP_LOGE(TAG, "Can't allocate downstreamPacket structure" COMMAND); \
            return ESP_ERR_NO_MEM; \
        } \
        pkt->buffer = calloc(strlen(buff) + 1, sizeof(char)); \
        if(pkt->buffer == NULL) { \
            ESP_LOGE(TAG, "Can't allocate char buffer for JSIN2Str" COMMAND); \
            return ESP_ERR_NO_MEM; \
        } \
        strcpy(pkt->buffer,buff); \
        pkt->len = strlen(pkt->buffer); \
        cJSON_Delete(JSON_ROOT_OBJECT_NAME);
*/
#define JSON_TO_DOWNSTREAM_BUFFER(JSON_ROOT_OBJECT_NAME,COMMAND) \
        char *buff=cJSON_Print(JSON_ROOT_OBJECT_NAME); \
        if(buff == NULL) { \
            ESP_LOGE(TAG, "Can't allocate buffer space from JSON for message " COMMAND); \
            return ESP_ERR_NO_MEM; \
        } \
        *downstreamPacket = malloc(sizeof(downstreamPkt)); \
        if((*downstreamPacket) == NULL) { \
            ESP_LOGE(TAG, "Can't allocate downstreamPacket structure" COMMAND); \
            return ESP_ERR_NO_MEM; \
        } \
        (*downstreamPacket)->buffer = calloc(strlen(buff) + 1, sizeof(char)); \
        if((*downstreamPacket)->buffer == NULL) { \
            ESP_LOGE(TAG, "Can't allocate char buffer for JSIN2Str" COMMAND); \
            return ESP_ERR_NO_MEM; \
        } \
        strcpy((*downstreamPacket)->buffer,buff); \
        (*downstreamPacket)->len = strlen((*downstreamPacket)->buffer); \
        cJSON_Delete(JSON_ROOT_OBJECT_NAME);


 //       char * buff=cJSON_Print(statusMsgObj); 
 //       if(buff == NULL) { 
  //          ESP_LOGE(TAG, "Can't allocate buffer space from JSON for message " "STATUS"); 
    //        return ESP_ERR_NO_MEM; 
     //   } 
     //   ESP_LOGI(TAG, "Created JSON Msg: %s",buff);
//        downstreamPacket_t pkt = *downstreamPacket;

   //     pkt = malloc(sizeof(downstreamPkt));

  //      pkt->buffer = calloc(strlen(buff) + 1, sizeof(char));
    //    if(pkt->buffer == NULL) { 
      //      ESP_LOGE(TAG, "Can't allocate char buffer for JSIN2Str" "STATUS");
    //        return ESP_ERR_NO_MEM; 
     //   } 
      //  strcpy(pkt->buffer,buff); 
    //    pkt->len = strlen(pkt->buffer); 
      //  cJSON_Delete(statusMsgObj);


/*
    please note the downstreamPacketBuffer has to be freed externally !
*/
esp_err_t interpreteCmd(char *inputBuffer, size_t inputBufferLen, downstreamPacket_t *downstreamPacket, sessionContext_t sessContext) {

    (*downstreamPacket) = NULL;
    cJSON *json = NULL;

    if(buffer2JSON(inputBuffer, inputBufferLen, &json) == ESP_FAIL) return ESP_FAIL;

    cJSON *cmd = cJSON_GetObjectItemCaseSensitive(json, "cmd");


    IF_JSON_CMD("PONG") {
        ESP_LOGI(TAG, "Got PONG Response");
        return ESP_OK;
    }

    IF_JSON_CMD("GETSTATUS") {
        ESP_LOGI(TAG, "Got Status Request");
        CREATE_JSON_COMMAND("STATUS",statusMsgObj);
        CREATE_JSON_BOOL_PARAMETER("auth",statusMsgObj,auth,sessContext->authenticated);
        JSON_TO_DOWNSTREAM_BUFFER(statusMsgObj, "STATUS");
        return ESP_OK;
    }

    IF_JSON_CMD("LOGIN") { 
        ESP_LOGI(TAG, "Got Login");

        cJSON *username = cJSON_GetObjectItemCaseSensitive(json, "username");
        cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

        CREATE_JSON_COMMAND("LOGIN",loginMsgObj);

        sessContext->authenticated = (strcmp(username->valuestring,  "admin")  == 0  && strcmp(password->valuestring,  "password")  == 0);

        CREATE_JSON_BOOL_PARAMETER("auth",loginMsgObj,auth,sessContext->authenticated);
        JSON_TO_DOWNSTREAM_BUFFER(loginMsgObj, "LOGIN");
        return ESP_OK;
    }

    IF_JSON_CMD("LOGOUT") { 
        ESP_LOGI(TAG, "Got LOGOUT");
        sessContext->authenticated = false;
        CREATE_JSON_COMMAND("LOGIN",loginMsgObj);
        CREATE_JSON_BOOL_PARAMETER("auth",loginMsgObj,auth,false);
        JSON_TO_DOWNSTREAM_BUFFER(loginMsgObj, "LOGIN");
        return ESP_OK;
    }

    return ESP_FAIL;
}


void freeDownstreamPacketBuffer(downstreamPacket_t downstreamPacket) {
    if(downstreamPacket != NULL) {
        if(downstreamPacket->buffer != NULL) {
            free(downstreamPacket->buffer);
        }
        free(downstreamPacket);
    }
}

/*

#define IF_JSON_CMD(CMD_TO_COMPARE)  if(ws_pkt.type == HTTPD_WS_TYPE_TEXT && cJSON_IsString(cmd) && (cmd->valuestring != NULL) && strcmp(cmd->valuestring,  CMD_TO_COMPARE) == 0 ) 




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
*/

