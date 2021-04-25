/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * Here are the defintions and prototypes for the Command Handler
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */

#pragma once


#include "websocket.h"



#define IF_JSON_CMD(CMD_TO_COMPARE)  if(cJSON_IsString(cmd) && (cmd->valuestring != NULL) && strcmp(cmd->valuestring,  CMD_TO_COMPARE) == 0) 


#define COMMAND_ASSERT_ALLOC_JSON(comparison,obj)   \
    if (comparison) {                               \
        ESP_LOGE(TAG, "Error creating JSON");       \
        cJSON_Delete(obj);                          \
        return ESP_FAIL;                            \
    }

#define CREATE_JSON_COMMAND(COMMAND,OBJECT_NAME)                        \
        cJSON *OBJECT_NAME = cJSON_CreateObject();                      \
        COMMAND_ASSERT_ALLOC_JSON(OBJECT_NAME == NULL,OBJECT_NAME);     \
        cJSON *cmd = cJSON_CreateString(COMMAND);                       \
        COMMAND_ASSERT_ALLOC_JSON(cmd == NULL,OBJECT_NAME);             \
        cJSON_AddItemToObject(OBJECT_NAME, "cmd", cmd);

#define CREATE_JSON_BOOL_PARAMETER(PARAMETER_NAME,JSON_ROOT_OBJECT_NAME,OBJECT_NAME,VALUE)  \
        cJSON *OBJECT_NAME = cJSON_CreateBool(VALUE);                                       \
        cJSON_AddItemToObject(JSON_ROOT_OBJECT_NAME, PARAMETER_NAME, OBJECT_NAME);


#define JSON_TO_DIRECT_ASYNC_CALLBACK(JSON_ROOT_OBJECT_NAME,COMMAND)                        \
        char *buff=cJSON_Print(JSON_ROOT_OBJECT_NAME);                                      \
        if(buff == NULL) {                                                                  \
            ESP_LOGE(TAG, "Can't allocate buffer space from JSON for message " COMMAND);    \
            return ESP_ERR_NO_MEM;                                                          \
        }                                                                                   \
        arg->msg = calloc(strlen(buff) + 1, sizeof(char));                                  \
        if(arg->msg == NULL) {                                                              \
            ESP_LOGE(TAG, "Can't allocate char buffer for JSIN2Str" COMMAND);               \
            return ESP_ERR_NO_MEM;                                                          \
        }                                                                                   \
        strcpy(arg->msg, buff);                                                             \
        arg->lastMessage = true;                                                            \
        arg->len = strlen(arg->msg);                                                        \
        cJSON_Delete(JSON_ROOT_OBJECT_NAME);                                                \
        callback(arg);


esp_err_t interpreteCmd(char *inputBuffer, size_t inputBufferLen, async_resp_arg_t arg, sessionContext_t sessContext, asyncCallback_t callback);

