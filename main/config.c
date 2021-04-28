
#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <string.h>

#include "mbedtls/md.h"
#include "cJSON.h"
#include "config.h"


static const char *TAG = "Esp32MotherClock.cfg";

static cJSON *mapJSON = NULL;
static cJSON *confJSON = NULL;



char *passwordToHash(char *pw) {
	unsigned char shaByteResult[32];
	char *shaHexResult;
    if((shaHexResult = (char*)calloc(65, sizeof(char))) == NULL) {                                                              
        ESP_LOGE(TAG, "Can't allocate buffer for hash");
        return NULL;
    } 
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
    mbedtls_md_init (&ctx);
    mbedtls_md_setup (&ctx, mbedtls_md_info_from_type (md_type), 0);
    mbedtls_md_starts (&ctx);
    mbedtls_md_update (&ctx, (const unsigned char *) pw, strlen (pw));
    mbedtls_md_finish (&ctx, shaByteResult);
    mbedtls_md_free (&ctx);
 
    for (int i = 0; i < 32; i++) sprintf (shaHexResult + 2 * i, "%02x", (int) shaByteResult [i]);

    return shaHexResult;
}


void replaceEscapes(char *destination, char *source, size_t len) {
    int j = 0;
    for (int i = 0; i <= len; i++) {
        if(source[i] == '\\' && source[i +1] == 'n') {
            destination[j] = '\n';
            ++j;
            ++i;
        } else if (source[i] == '\\' && source[i +1] == 'r') {
            destination[j] = '\r';
            ++j;
            ++i;
        }  else if (source[i] == '\\' && source[i +1] == 't') {
            destination[j] = '\t';
            ++j;
            ++i;
        } else {
            destination[j] = source[i];
            ++j;
        }
    }
}


int getTypeFromItem(cJSON *item) {
	cJSON *itemType = cJSON_GetObjectItem(item, "type");

	if(itemType != NULL) {
		if(strncmp(itemType->valuestring,"string",10) == 0) {
			return CONF_STRING_TYPE;
		} else if(strncmp(itemType->valuestring,"number",10) == 0) {
			return CONF_NUMBER_TYPE;
		} else if(strncmp(itemType->valuestring,"bool",10) == 0) {
			return CONF_BOOL_TYPE;
		}
	} else {
		switch(item->type) {
			case cJSON_NULL:
				return CONF_NULL_TYPE; 		break;
			case cJSON_False:
			case cJSON_True:
				return CONF_BOOL_TYPE; 		break;
			case cJSON_String:
				return CONF_STRING_TYPE; 	break;
			case cJSON_Number:
				return CONF_NUMBER_TYPE; 	break;
			default: break;
		}
	}

	return CONF_UNKNOWN_TYPE;
}

static cJSON *fileToJSON(char *filename) {
	char *buffer;
	cJSON *retJSON = NULL;
	FILE *f= NULL;
	struct stat stats;

	ESP_LOGI(TAG, "Reading config file: %s", filename);

    if(stat(filename, &stats) == -1) {
    	ESP_LOGW(TAG, "File %s may not exists", filename);
        return NULL;
    }

    if((buffer = (char*)calloc(stats.st_size + 1, sizeof(char))) == NULL) {                                                              
        ESP_LOGE(TAG, "Can't allocate buffer for map file in conf");
        return NULL;
    } 

	if ((f = fopen(filename, "r")) == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", filename);
        return NULL;
    }

    size_t read_bytes = fread(buffer,sizeof(char), stats.st_size, f);
    if(read_bytes != stats.st_size) {
		ESP_LOGE(TAG, "Error - buffer read smaller than file");
		fclose(f);
		free(buffer);
		return NULL;
    } 

	fclose(f);

	retJSON = cJSON_ParseWithLength(buffer,stats.st_size);

    if (!retJSON) {
        ESP_LOGE(TAG, "Error inf file %s before: [%s]\n", filename, buffer); //cJSON_GetErrorPtr()
        return NULL;
    }
    free(buffer);    
    return retJSON;
}



static esp_err_t initEmptyConf() {
	ESP_LOGW(TAG, "Init Empty Conf");
	confJSON = cJSON_CreateObject();

    if (confJSON == NULL) {
        ESP_LOGE(TAG, "Error creating JSON");
        cJSON_Delete(confJSON);
        return ESP_FAIL;
    }

    cJSON *current_element = NULL;
	char *current_key = NULL;

	cJSON *mapItems = cJSON_GetObjectItem(mapJSON, "items");

	cJSON_ArrayForEach(current_element, mapItems) {
    	current_key = current_element->string;
    	if (current_key != NULL) {
			cJSON *mapSingleItem = cJSON_GetObjectItem(mapItems, current_key);

			if(cJSON_IsTrue(cJSON_GetObjectItem(mapSingleItem, "relevant"))) {

				cJSON *itemDefault = cJSON_GetObjectItem(mapSingleItem, "default");

				cJSON *confObjItem = NULL;

				switch(getTypeFromItem(mapSingleItem)) {
					case CONF_BOOL_TYPE: 
						confObjItem = cJSON_IsTrue(itemDefault) ? cJSON_CreateTrue() : cJSON_CreateFalse(); 
						break; 
					case CONF_STRING_TYPE:
						if(strncmp(current_key, "password",8) == 0) {	/* special threatment of Passwords, the excepetion is the rule */
							char *hashedPW = passwordToHash("password");
							if(hashedPW != NULL) {
								confObjItem = cJSON_CreateString(hashedPW);
								free(hashedPW);
							} else ESP_LOGE(TAG, "Can't set default PW");
							
							break;	
						} 
						confObjItem = cJSON_CreateString(itemDefault ? itemDefault->valuestring : "");
						break;
					case CONF_NUMBER_TYPE:
						confObjItem = cJSON_CreateNumber(itemDefault ? itemDefault->valuedouble: 0);
						break;
					default:
						confObjItem = cJSON_CreateString("");
						break;
				}
			
	        	if(confObjItem) {	
	        		if (confObjItem == NULL) {
	        			ESP_LOGE(TAG, "Error creating JSON");
	        			cJSON_Delete(confObjItem);
	        			return ESP_FAIL;
	    			}
					cJSON_AddItemToObject(confJSON, current_key, confObjItem);
				} else ESP_LOGW(TAG, "Could not load default value");
			
			}

    	}
	}

	return ESP_OK;
}

cJSON *getConfByKey(char * key) {
	if(confJSON == NULL) return NULL;

	return cJSON_GetObjectItem(confJSON, key);

}

#define GET_BY_KEY_AND_CHECK(TYPE,RETURN) 												\
	const char *typeName[] = {"Unknown", "Null", "Bool", "String", "Number"}; 	\
	cJSON *p = getConfByKey(key); 												\
	if(p == NULL) { 															\
		ESP_LOGE(TAG, "Unknown %s parameter %s", typeName[TYPE], key);			\
		return RETURN;															\
	}																			\
	if(getTypeFromItem(p) != TYPE) {											\
		ESP_LOGE(TAG, "Asking to get %s parameter %s for parameter is not %s", typeName[TYPE], key, typeName[TYPE]); \
		return RETURN;															\
	}


bool confToBoolByKey(char * key) {
	GET_BY_KEY_AND_CHECK(CONF_BOOL_TYPE, false);
	return cJSON_IsTrue(p) ? true : false;
}
char *confToStringByKey(char * key) {
	GET_BY_KEY_AND_CHECK(CONF_STRING_TYPE, NULL);
	return p->valuestring;
}
double confToDoubleByKey(char * key) {
	GET_BY_KEY_AND_CHECK(CONF_NUMBER_TYPE, 0);
	return p->valuedouble;
}
int confToIntByKey(char * key) {
	GET_BY_KEY_AND_CHECK(CONF_NUMBER_TYPE, 0);
	return p->valuedouble;
}



void setConfigByKey(char *key, cJSON *value) {

	if(key == NULL) return;
	cJSON *configEntry = getConfByKey(key);

	if(configEntry == NULL) return;
	if(value == NULL) return;


	switch(getTypeFromItem(value)) {
		case CONF_BOOL_TYPE: 
			configEntry->type = cJSON_IsTrue(value) ? cJSON_True : cJSON_False;
			break; 
		case CONF_STRING_TYPE:
			if(strncmp(key, "password",8) == 0) {	/* special threatment of Passwords, the excepetion is the rule */
				char *hashedPW = passwordToHash(value->valuestring);
				if(hashedPW != NULL) {
					cJSON_SetValuestring(configEntry, hashedPW);
					free(hashedPW);
				} else ESP_LOGE(TAG, "Can't set PW");
				break;	
			} 
			cJSON_SetValuestring(configEntry, value->valuestring);
			break;
		case CONF_NUMBER_TYPE:
			cJSON_SetNumberValue(configEntry, value->valuedouble);
			break;
		default:
			break;
	}
}

esp_err_t initConf(void) {
	if(! (mapJSON = fileToJSON(MAPFILE))) {
		return ESP_FAIL;
	}
	
	if(! (confJSON = fileToJSON(CONFIGFILE))) {
		initEmptyConf();
	}

 	ESP_LOGI(TAG, "Empty Conf: %s", cJSON_Print(confJSON));

	return ESP_OK;
}



esp_err_t commitConf(void) {
	char * buffer = cJSON_Print(confJSON);
	if(buffer == NULL) return ESP_FAIL;

	ESP_LOGI(TAG, "%s", cJSON_Print(confJSON));

	FILE *f= NULL;

	if ((f = fopen(CONFIGFILE,"w")) == NULL) {
        ESP_LOGE(TAG, "Failed to open %s", CONFIGFILE);
        return ESP_FAIL;
    }

	fprintf(f,"%s",buffer);

	fclose(f);

	return ESP_OK;
}

void destroyConf(void) {
	if(mapJSON)  cJSON_Delete(mapJSON);
	if(confJSON) cJSON_Delete(confJSON);	
}




