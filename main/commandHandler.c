/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * This handles any incomming command via WebSocket
 * A callback is called in case data has to be send back to WS
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */

#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <string.h>
#include "freertos/task.h"
#include <unistd.h>
#include <sys/fcntl.h>
#include <esp_http_server.h>

#include "cJSON.h"

#include "commandHandler.h"

#include "websocket.h"



static const char *TAG = "Esp32MotherClock.wsCmd";

#define SCRATCH_BUFSIZE (10240)
#define MAPFILE "/etc/configMap.json"

static char scratch[SCRATCH_BUFSIZE];

 
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






/*
    please note the downstreamPacketBuffer has to be freed externally !
*/
// esp_err_t interpreteCmd(char *inputBuffer, size_t inputBufferLen, downstreamPacket_t *downstreamPacket, sessionContext_t sessContext) {
esp_err_t interpreteCmd(char *inputBuffer, size_t inputBufferLen, async_resp_arg_t arg, sessionContext_t sessContext, asyncCallback_t callback) {

//    (*downstreamPacket) = NULL;
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
        JSON_TO_DIRECT_ASYNC_CALLBACK(statusMsgObj, "STATUS");
        return ESP_OK;
    }

    IF_JSON_CMD("LOGIN") { 
        ESP_LOGI(TAG, "Got Login");

        cJSON *username = cJSON_GetObjectItemCaseSensitive(json, "username");
        cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

        CREATE_JSON_COMMAND("LOGIN",loginMsgObj);

        sessContext->authenticated = (strcmp(username->valuestring,  "admin")  == 0  && strcmp(password->valuestring,  "password")  == 0);

        CREATE_JSON_BOOL_PARAMETER("auth",loginMsgObj,auth,sessContext->authenticated);
        JSON_TO_DIRECT_ASYNC_CALLBACK(loginMsgObj, "LOGIN");

        return ESP_OK;
    }

    IF_JSON_CMD("LOGOUT") { 
        ESP_LOGI(TAG, "Got LOGOUT");
        sessContext->authenticated = false;
        CREATE_JSON_COMMAND("LOGIN",loginMsgObj);
        CREATE_JSON_BOOL_PARAMETER("auth",loginMsgObj,auth,false);
        JSON_TO_DIRECT_ASYNC_CALLBACK(loginMsgObj, "LOGIN");
        return ESP_OK;
    }

    /* all other commands below will require authentication */
    /* so we quit here in case no auth is set for this session */
    if(!sessContext->authenticated) {                       
        ESP_LOGW(TAG, "command either only possible beeing authenticated or invalid");
        return ESP_FAIL;
    }


    IF_JSON_CMD("GETMAP") { 
        ESP_LOGI(TAG, "Got GETMAP");


        int fd = open(MAPFILE, O_RDONLY, 0);
        if (fd == -1) {
            ESP_LOGE(TAG, "Failed to open " MAPFILE);
            return ESP_FAIL;
        }


        struct stat stats;

        stat(MAPFILE, &stats);
        size_t fileLen =  stats.st_size; 
        size_t sentLen = 0;
        int packetNo = 0;


        snprintf(scratch, sizeof(scratch), "{ \"cmd\": \"GETMAP\", \"totallen\": %d, \"len\": %d, \"packetNo\": %d,\"data\": ", fileLen, SCRATCH_BUFSIZE, SCRATCH_BUFSIZE);
        unsigned int overheadLen = strnlen(scratch, SCRATCH_BUFSIZE) + 2; // +2 for the ending parenthesis and NULL

        ssize_t read_bytes;
        do {
            read_bytes = read(fd,  &scratch, SCRATCH_BUFSIZE - overheadLen);
            if (read_bytes == -1) {
                ESP_LOGE(TAG, "Failed to read file : " MAPFILE);
            } else if (read_bytes > 0) {

                char *buffer = calloc(strlen(scratch) + overheadLen + 4, sizeof(char));
                if(buffer == NULL) {                                                              
                    ESP_LOGE(TAG, "Can't allocate buffer for download map file");
                    return ESP_ERR_NO_MEM;
                } 

                for(int i=0;scratch[i]!='\0';i++) {
                    if(scratch[i] == '"') scratch[i] = '\'';   // cheap method to escape the quotes 
                    if(scratch[i] == '\n') scratch[i] = ' ';   
                    if(scratch[i] == '\t') scratch[i] = ' ';   
                }

                snprintf(buffer,sizeof(scratch) + overheadLen + 4,  "{ \"cmd\": \"GETMAP\", \"totallen\": %d, \"len\": %d, \"packetNo\": %d, \"data\": \"%s\" }", fileLen, read_bytes,packetNo, scratch);
                ++packetNo;

                sentLen = sentLen + read_bytes;


                if(sentLen >= fileLen) { // shouldn't be larger 
                    arg->lastMessage = true; 
                }  else {
                    arg->lastMessage = false; 
                }
                arg->msg = buffer; 
                arg->len = strlen(buffer); 
                callback(arg);

            }
        } while (read_bytes > 0);
        


        close(fd);


        return ESP_OK;
    }


    return ESP_FAIL;
}


