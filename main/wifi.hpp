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
#include <WebServer.h>
#include <Arduino.h>

#define MAX_WIFI_CONN_RETRIES 10
#define WIFI_CONN_TIMEOUT_MS 10000

#define SERVER_PORT 2926


extern WebServer server;


void wifiSetup();


void wifiEventHandler();



#endif
