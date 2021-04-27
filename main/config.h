

#pragma once

#include "cJSON.h"

#define MAPFILE "/etc/configMap.json"
#define CONFIGFILE "/etc/configuration.json"

#define CONF_UNKNOWN_TYPE 	0
#define CONF_NULL_TYPE 		1
#define CONF_BOOL_TYPE 		2
#define CONF_STRING_TYPE 	3
#define CONF_NUMBER_TYPE 	4

int getTypeFromItem(cJSON *item);
esp_err_t initConf(void);
esp_err_t commitConf(void);
cJSON *getConfByKey(char * key);
void setConfigByKey(char *key, cJSON *value);

