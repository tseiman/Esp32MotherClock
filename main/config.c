
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <unistd.h>
#include "cJSON.h"

#include "commandHandler.h"

static const char *TAG = "Esp32MotherClock.cfg";

static cJSON *mapJSON = NULL;
static cJSON *confJSON = NULL;


static cJSON *fileToJSON(char *filename) {
	char *buffer;
	cJSON *retJSON;
	struct stat stats;

    stat(filename, &stat);

 	if ((fd = open(MAPFILE, O_RDONLY, 0)) == -1) {
        ESP_LOGE(TAG, "Failed to open %s", filename);
        return NULL;
    }

    if((buffer = (char*)malloc(stat.st_size + 1)) == NULL) {                                                              
        ESP_LOGE(TAG, "Can't allocate buffer for map file in conf");
        return NULL;
    } 

    int read_bytes = read(fd, buffer, stat.st_size);
    close(fd);

	mapJSON = cJSON_Parse(buffer);
	free(buffer);
    if (!mapJSON) {
        ESP_LOGE(TAG, "Error inf file %s before: [%s]\n", filename, cJSON_GetErrorPtr());
        return NULL;
    }
    return retJSON;
}


static esp_err_t initEmptyConf() {
	confJSON = cJSON_CreateObject();
    COMMAND_ASSERT_ALLOC_JSON(confJSON == NULL,confJSON);

    cJSON *current_element = NULL;
	char *current_key = NULL;

	cJSON *mapItems = cJSON_GetObjectItem(mapJSON, "items");

	cJSON_ArrayForEach(current_element, mapItems) {
    	current_key = current_element->string;
    	if (current_key != NULL) {
			cJSON *mapSingleItem = cJSON_GetObjectItem(mapItems, current_key);
			cJSON *itemDefault = cJSON_GetObjectItem(mapSingleItem, "default");

			cJSON *confObjItem = NULL;
			
			if(cJSON_IsString(itemDefault)) {
        		confObjItem = cJSON_CreateString(itemDefault ? itemDefault->valuestring : "");
			} else if (cJSON_IsNumber(itemDefault)) {
        		confObjItem = cJSON_CreateNumber(itemDefault ? itemDefault->valuedouble: 0);
			} else if (cJSON_IsTrue(itemDefault)) {
        		confObjItem = cJSON_CreateTrue();
			} else if (cJSON_IsFalse(itemDefault)) {
        		confObjItem = cJSON_CreateFalse();
			} 
        	if(confObjItem) {	
        		COMMAND_ASSERT_ALLOC_JSON(confObjItem == NULL,confObjItem);
				cJSON_AddItemToObject(confJSON, current_key, confObjItem);
			} else {
				ESP_LOGW(TAG, "Could not load default value");
			}

    	}
	}

	return ESP_OK;
}

esp_err_t initConf(void) {
	if(! (mapJSON = fileToJSON(MAPFILE))) {
		return ESP_FAIL;
	}
	if(! (confJSON = fileToJSON(CONFIGFILE))) {
		initEmptyConf();
	}



	return ESP_OK;
}



esp_err_t commitConf(void) {
	return ESP_OK;
}

void destroyConf(void) {
	if(mapJSON)  cJSON_Delete(mapJSON);
	if(confJSON) cJSON_Delete(confJSON);	
}




