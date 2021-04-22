/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * Filesystem IO should go here - basically it initializes SPIFFS
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */


#include <esp_log.h>

#include "esp_vfs_fat.h"
#include "esp_spiffs.h"

#include "filesystem.h"

#define WEB_MOUNT_POINT "/www"
#define ETC_MOUNT_POINT "/etc"

static const char *TAG = "Esp32MotherClock.fs";



static esp_err_t init_fs(esp_vfs_spiffs_conf_t *conf) {

    ESP_LOGI(TAG, "Mounting %s", conf->base_path);

    esp_err_t ret = esp_vfs_spiffs_register(conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
         return ESP_FAIL;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf->partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}


esp_err_t mount(void) {
    ESP_LOGI(TAG, "Init filesystem");

    int ret = ESP_OK;

    esp_vfs_spiffs_conf_t conf_www = {
        .base_path = WEB_MOUNT_POINT,
        .partition_label = "www",
        .max_files = RESOURCES_LIST_WWW_FILE_COUNT,
        .format_if_mount_failed = false
    };

    if((ret = init_fs(&conf_www)) != ESP_OK) {
        return ret;
    }


    esp_vfs_spiffs_conf_t conf_etc = {
        .base_path = ETC_MOUNT_POINT,
        .partition_label = "etc",
        .max_files = RESOURCES_LIST_ETC_FILE_COUNT,
        .format_if_mount_failed = false
    };

    ret = init_fs(&conf_etc);

    return ESP_OK;
}



