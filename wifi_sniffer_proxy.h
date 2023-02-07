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
#define MAX_FRAME_BODY_LENGHT 255
#define WIFI_CHANNEL_SWITCH_INTERVAL  (500)
#define MAX_DEVICE_TRACKED 40

#define DOT11_FRAME_BODY_ELEMENT_ID_SSID 0 // NOT // id(1)-length(1)-SSID(0-32)
#define DOT11_FRAME_BODY_ELEMENT_ID_DSSS 3 // id(1)-length(1)-channel(1)
#define DOT11_FRAME_BODY_ELEMENT_ID_SUPPORTED_RATES 1 // id(1)-length(1)-supported rates(1-8)
#define DOT11_FRAME_BODY_ELEMENT_ID_HT_CAPABILITIES 45 // id(1)-length(1)-ht capability info(2)-A-MPDU(1)-MCS SET(16)-HT extended(2)-Transmit Beamforming (4)-ASEL (1)
#define DOT11_FRAME_BODY_ELEMENT_ID_SUPPORTED_RATES_EXTENDED 50 // id(1)-length(1)-Extended Support Rates(1-255)
#define DOT11_FRAME_BODY_ELEMENT_ID_SUPPORTED_OPERATING_CLASSES 59 // NOT // id(1)-length(1)-Current Operating Class(1)-Current Operating Class Extension(var, opt), Operating Class Duple Sequence(var, opt)
	// Current Operating Class Extension - oddělovač 130 - položky
	// Operating Class Duple Sequence - odělovač 00 - 2xpočet položek v Operating Class Extension
#define DOT11_FRAME_BODY_ELEMENT_ID_2040COEX 72 // id(1)-length(1)-20/40 BSS Coexistence(1)
	// inforamtion request(1), 40 MHz toleratnt(1), 20MHz BSS Width Request (1), OBSS Scanning Exemption Request (1), OBSS Scanning Exemption Grant (1), Reserved (1)
#define DOT11_FRAME_BODY_ELEMENT_ID_EXTENDED_CAPABILITES 127 // NOT // id(1)-length(1)-Extended Capabilities(88 bitů)



static const uint8_t WiFiChannels[3] = {1,6,11};

typedef struct{
	uint8_t dsss_channel;
	uint8_t supported_rates[8];
	uint16_t ht_capability_info;
	uint8_t ht_a_mpdu_param;
	uint16_t ht_extended_cap;
	uint32_t ht_transmit_beamforming_cap;
	uint8_t ht_ansel_cap;
	uint8_t coex_20_40;
} wifi_ieee80211_probe_reqest_body_parsed_t;

typedef struct{
	wifi_ieee80211_probe_reqest_body_parsed_t body;
  uint8_t addr[6];
	uint64_t lastFrame;
	uint16_t innerBurstSpace[20]; // delta between current time and time of last frame

} burst_frame_parsed_t;

typedef struct{
  uint32_t LSHsigniture;
  uint16_t innerBurstDelay; // switch to
  uint8_t lastUnderMAC[6];
	uint8_t appearedUnderNoMAC;

  /* uint64_t timestampBuffer[arraySize]; // switch to */
  /* uint8_t index = 1; */
  /* uint8_t start = 0; */
  /* uint8_t alertRaised = 0; */
} device_log_t;



static uint8_t level = 0, channel = 0;
static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 13, .policy=WIFI_COUNTRY_POLICY_AUTO}; //Most recent esp32 library struct


// formwat of general MAC frame header
typedef struct {
  uint16_t frame_ctrl;
  uint16_t duration_id;
  uint8_t dest_addr[6]; /* receiver address */
  uint8_t send_addr[6]; /* sender address */
  uint8_t bssid_addr[6]; /* filtering address */
  uint16_t sequence_ctrl;
  //uint8_t ht_control[4]; // seems redudnadat for probe request

} wifi_ieee80211_mgmt_hdr_t;


typedef struct {
  wifi_ieee80211_mgmt_hdr_t hdr;
  uint8_t body[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_mgmt_packet_t;


class WiFiSnifferProxy{
  private:
    static WiFiSnifferProxy* instance;

    bool active = true;
    bool error = false;

    //deviceLog devices[MAX_DEVICE_TRACKED];
		burst_frame_parsed_t burstFrames[MAX_DEVICE_TRACKED];

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
