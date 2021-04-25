

#pragma once

#define MAPFILE "/etc/configMap.json"
#define CONFIGFILE "/etc/configuration.json"


esp_err_t initConf();
esp_err_t commitConf();