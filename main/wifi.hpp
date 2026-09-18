/**
 * @file wifi.hpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-18
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#ifndef __WIFI_HPP__
#define __WIFI_HPP__

#include <WiFi.h>
#include <secrets.h>

#define MAX_WIFI_CONN_RETRIES 10
#define WIFI_CONN_TIMEOUT_MS 10000


void wifiSetup() {
  Serial.printf("Connecting to WiFi network %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  for (int i=0; i<MAX_WIFI_CONN_RETRIES; ++i) {
    Serial.printf("Attempt %d/%d | %d\n", i+1, MAX_WIFI_CONN_RETRIES, WiFi.status());

    WiFi.setTxPower(WIFI_POWER_8_5dBm);

    WiFi.disconnect(true);
    delay(100);

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    uint32_t elapsed_ms = 0;
    uint32_t prev = millis();
    
    while (WiFi.status() != WL_CONNECTED && elapsed_ms < WIFI_CONN_TIMEOUT_MS) {
      uint32_t now = millis();

      elapsed_ms += (now - prev);
      prev = now;

      delay(500);
    }

    if (WiFi.status() == WL_CONNECTED) {
      break;
    }
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.printf("WiFi connection failed.\n");
    abort();
  }
  Serial.printf("WiFi connected!\n");
}



#endif
