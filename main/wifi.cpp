/**
 * @file wifi.cpp
 * @author Poh Jing Seng (hello@jspoh.dev)
 * @brief 
 * @version 0.1
 * @date 2026-09-18
 * 
 * @copyright Copyright (c) 2026
 * 
 */


#include "wifi.hpp"
#include "tx.hpp"


WebServer server(SERVER_PORT);
WebServer webui_server(WEBUI_PORT);


void setupRoutes() {
#ifdef DISABLE_WIFI
  return;
#endif

  server.on("/tx", HTTP_GET, [](){
    bool success = false;
    String cmdStr = server.arg("cmd");
    const char cmd = cmdStr[0];
    // fixed-code remotes (raw replay)
    for (const auto& [trigger, td] : TX_CONFIG) {
      if (cmd == trigger) {
        Serial.printf("Tx request from network received: %c\n", cmd);
        txPulses(td.pulse_binary.c_str(), td.long_pulse_us, td.short_pulse_us, td.pulse_gap_us, 8);
        Serial.printf("Tx trigger with %c completed\n", trigger);
        success = true;
        break;
      }
    }

    // rolling-code remotes (Nice Flor-S): advance the persisted counter, then synthesise
    if (!success) {
      for (const auto& r : NICE_REMOTES) {
        if (cmd == r.trigger) {
          const uint16_t counter = niceCounterNext(r.serial, r.seed_counter);
          Serial.printf("Tx Nice Flor-S from network: %c serial=0x%07X counter=%u btncode=0x%X\n", cmd, r.serial, counter, r.btncode);
          txNiceFlorS(r.serial, counter, r.btncode, 1);   // 1 burst == one press (one tap)
          Serial.printf("Tx Nice Flor-S %c completed\n", cmd);
          success = true;
          break;
        }
      }
    }

    server.send(success ? 200 : 400, "text/plain", success ? "ok" : "malformed. Usage: <ip>:<port>/tx?cmd=<byte>");
  });

  webui_server.on("/", HTTP_GET, []() {
    webui_server.send(200, "text/html", WEBUI_TEMPLATE.c_str());
  });
}


void wifiSetup() {
#ifdef DISABLE_WIFI
  return;
#endif

  Serial.printf("Connecting to WiFi network %s\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);   // disable modem power-save: cuts response latency, stops stalls
  for (int i=0; i<MAX_WIFI_CONN_RETRIES; ++i) {
    Serial.printf("Attempt %d/%d | %d\n", i+1, MAX_WIFI_CONN_RETRIES, WiFi.status());

    WiFi.setTxPower(WIFI_POWER_7dBm);

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
    Serial.printf("WiFi connection failed: %d\n", WiFi.status());
    abort();
  }
  Serial.printf("WiFi connected!\n");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  Serial.printf("Setting up server routes..\n");
  setupRoutes();
  Serial.printf("Done.\n");

  Serial.printf("Starting servers..\n");
  server.begin();
  webui_server.begin();
  Serial.printf("Done.\n");
}


void wifiEventHandler() {
#ifdef DISABLE_WIFI
  return;
#endif

  server.handleClient();
  webui_server.handleClient();
}
