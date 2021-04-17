
/* ***************************************************************************
 *
 * Thomas Schmidt, 2021
 *
 * This file is part of the Esp32MotherClock Project
 *
 * The file contains the static HTTPS header
 * 
 * License: MPL-2.0 License
 *
 * Project URL: https://github.com/tseiman/Esp32MotherClock
 *
 ************************************************************************** */
#pragma once


httpd_handle_t start_static_webserver(void);

void stop_static_webserver(httpd_handle_t server);
void registerStaticResources(httpd_handle_t server);