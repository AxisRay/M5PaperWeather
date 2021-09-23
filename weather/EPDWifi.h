/*
   Copyright (C) 2021 SFini

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/**
  * @file EPDWifi.h
  * 
  * Helper functions for the wifi connection.
  */
#pragma once
#include <WiFi.h>
#include "Config.h"

#ifdef WPA2_EAP_ID
#include "esp_wpa2.h"
#include "esp_wifi.h"
#include "esp_wpa2.h"
#endif

/* Start and connect to the wifi */
bool StartWiFi(int &rssi)
{
   IPAddress dns(8, 8, 8, 8); // Google DNS

   WiFi.mode(WIFI_STA);
   WiFi.disconnect();
   WiFi.setAutoConnect(true);
   WiFi.setAutoReconnect(true);

   log_i("Connecting to %s",WIFI_SSID);
   delay(100);

#ifdef WPA2_EAP_ID
   esp_wpa2_config_t config = WPA2_CONFIG_INIT_DEFAULT();
   // This part of the code is taken from the oficial wpa2_enterprise example from esp-idf
   ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
   ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)WPA2_EAP_ID, strlen(WPA2_EAP_ID)));
   ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_username((uint8_t *)WPA2_EAP_USERNAME, strlen(WPA2_EAP_USERNAME)));
   ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_set_password((uint8_t *)WPA2_EAP_PASSWORD, strlen(WPA2_EAP_PASSWORD)));
   ESP_ERROR_CHECK(esp_wifi_sta_wpa2_ent_enable(&config));
   WiFi.begin(WIFI_SSID);
#else
   WiFi.begin(WIFI_SSID, WIFI_PW);
#endif
   for (int retry = 0; WiFi.status() != WL_CONNECTED && retry < 10; retry++)
   {
      delay(500);
      Serial.print(".");
   }

   rssi = 0;
   if (WiFi.status() == WL_CONNECTED)
   {
      rssi = WiFi.RSSI();
      log_i("WiFi connected at: %s", WiFi.localIP().toString());
      return true;
   }
   else
   {
      log_e("WiFi connection *** FAILED ***");
      return false;
   }
}

/* Stop the wifi connection */
void StopWiFi()
{
   Serial.println("Stop WiFi");
   WiFi.disconnect();
   WiFi.mode(WIFI_OFF);
}
