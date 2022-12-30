// vim: set ft=arduino:
/*
 * gas_sensor_proxy.h
 * Copyright (C) 2022 Kryštof Havránek <krystof@havrak.xyz>
 *
 * Distributed under terms of the MIT license.
 */

#ifndef WIFI_SNIFFER_PROXY_H
#define WIFI_SNIFFER_PROXY_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#define DATA_LENGTH           112
#define WIFI_CHANNEL_MAX               (13)
#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)

#include "log_config.h"


static uint8_t level = 0, channel = 1;
static wifi_country_t wifi_country = {.cc="CZ", .schan = 1, .nchan = 13}; //Most recent esp32 library struct
    
typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;


class WiFiSnifferProxy{
  private:
    static WiFiSnifferProxy* instance;

    bool active = true;
    bool error = false;
    deviceLog devices[MAX_DEVICE_TRACKED];

    WiFiSnifferProxy();

    uint64_t timeOfMeasurement;

    bool first = true;

    static esp_err_t eventHandler(void *ctx, system_event_t *event);
    static void wifiSnifferPacketHandler(void *buff, wifi_promiscuous_pkt_type_t type); 



  public:

    /**
     * main method to access WiFiSnifferProxy
     * if instance of proxy wasn't yet created
     * method will do the initialization
     */
    static WiFiSnifferProxy* getInstance();

    
    void switchChannel();

    void logBeaconedDeviceAddress(const uint8_t* address);

    bool checkForSuspiciousDevices();
    
    /**
     * returns whether periphery is active
     *
     * @return bool - periphery state
     */
    bool isActive(){
      return active;
    }

    /**
     * activates or deactivates periphery
     * when set to false sensor data will not be read
     *
     * @param bool active
     */
    void setActive(bool active){
      this->active = active;
    }


};

#endif /* !WIFI_SNIFFER__PROXY_H */
