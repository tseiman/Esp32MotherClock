/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * The file contains the header for filesystem IO
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */

#pragma once

#include "esp_vfs_fat.h"
#include "esp_spiffs.h"


esp_err_t mount(void);